/// @file
#include "hal_spi.hpp"

#include "hal.hpp"
#include "rtt.hpp"
#include <ads131m02/ADS131M02.hpp>
#include <ais2ih/AIS2IH.hpp>
#include <cstddef>
#include <freertos/binary_semaphore.hpp>
#include <freertos/timing.hpp>
#include <utils/byte_utils.hpp>
#include <stm32c0xx.h>
#include <stm32c0xx_ll_spi.h>
#include <utils/uncopyable.hpp>

/// Feel free to redefine to `true` when developing just the SPI driver
/// on `release` builds.
#define SPI_DEBUG() !defined(NDEBUG)

#if SPI_DEBUG()
    /// This is NOT error checking! These assertions are only used to ensure
    /// invariants when developing SPI driver and to help documenting them.
    #define SPI_ASSERT(reason, assertion)                                  \
        if (assertion) [[likely]] {                                        \
        } else {                                                           \
            rtt::print(reason);                                            \
            hal::panic(indx_head::errors::FaultStatusMask::assert_failed); \
        }

    /// Debug adds ~25% overhead to cycle count.
    #warning "SPI_DEBUG() must be disabled in production build"

#else
    #define SPI_ASSERT(...)
#endif

namespace hal::spi {
namespace {

    SPI_TypeDef *const SPIx = SPI1;
    DMA_TypeDef *const DMAx = DMA1;
    DMA_Channel_TypeDef *const DMA_CHAN_RX = DMA1_Channel6; // indexed from 1
    DMA_Channel_TypeDef *const DMA_CHAN_TX = DMA1_Channel7; // indexed from 1
    DMAMUX_Channel_TypeDef *const DMAMUX_CHAN_RX = DMAMUX1_Channel5; // indexed from 0
    DMAMUX_Channel_TypeDef *const DMAMUX_CHAN_TX = DMAMUX1_Channel6; // indexed from 0
    const uint32_t DMA_CPARx = (uint32_t)&SPIx->DR; // fixed peripheral address
    volatile uint32_t DMAx_ISR = 0; // written by interrupt, read by task, guarded by semaphore
    freertos::BinarySemaphore semaphore;

    struct TransmitReceiveParameters {
        const std::byte *tx_data;
        std::byte *rx_data;
        size_t size;
        GPIO_TypeDef *cs_port;
        uint32_t cs_pin;
    };

    [[nodiscard]] bool transmit_receive(const TransmitReceiveParameters &params) {
        const auto &[tx_data, rx_data, size, cs_port, cs_pin] = params;

        // caller must ensure these preconditions
        SPI_ASSERT("Empty transfer is not supported\n", size > 0);
        SPI_ASSERT("Need distinct buffers\n", rx_data != tx_data);
        SPI_ASSERT("DMA_CNDTRx can only fit 16 bits (RX and TX)\n", (size & 0xffff0000) == 0);

        // previous run (or init) must ensure these preconditions
        SPI_ASSERT("SPI must be disabled\n", READ_BIT(SPIx->CR1, SPI_CR1_SPE) == 0);
        SPI_ASSERT("DMA_CPARx never changes after init (RX)\n", READ_REG(DMA_CHAN_RX->CPAR) == DMA_CPARx);
        SPI_ASSERT("DMA_CPARx never changes after init (TX)\n", READ_REG(DMA_CHAN_TX->CPAR) == DMA_CPARx);
        SPI_ASSERT("DMA_CCRx must be writable (RX)\n", READ_BIT(DMA_CHAN_RX->CCR, DMA_CCR_EN) == 0);
        SPI_ASSERT("DMA_CCRx must be writable (TX)\n", READ_BIT(DMA_CHAN_TX->CCR, DMA_CCR_EN) == 0);

        // drive SCK pin to correct level
        SET_BIT(SPIx->CR1, SPI_CR1_SPE);

        // setup RX DMA channel
        // this takes ~10 cycles so SCK has time to settle
        WRITE_REG(DMA_CHAN_RX->CNDTR, size);
        WRITE_REG(DMA_CHAN_RX->CMAR, (uint32_t)rx_data);
        WRITE_REG(DMA_CHAN_RX->CCR, DMA_CCR_EN | DMA_CCR_MINC | DMA_CCR_TEIE | DMA_CCR_TCIE);

        // only drive CS pin _after_ SCK pin settled to prevent false clock transition
        WRITE_REG(cs_port->BRR, cs_pin);

        // setup TX DMA channel
        // this takes ~10 cycles so CS has time to settle
        WRITE_REG(DMA_CHAN_TX->CNDTR, size);
        WRITE_REG(DMA_CHAN_TX->CMAR, (uint32_t)tx_data);
        WRITE_REG(DMA_CHAN_TX->CCR, DMA_CCR_EN | DMA_CCR_MINC | DMA_CCR_TEIE | DMA_CCR_DIR);

        // wait for completion
        semaphore.acquire();

        // interrupt handler must ensure these preconditions
        SPI_ASSERT("DMA_CCRx must be writable (RX)\n", READ_BIT(DMA_CHAN_RX->CCR, DMA_CCR_EN) == 0);
        SPI_ASSERT("DMA_CCRx must be writable (TX)\n", READ_BIT(DMA_CHAN_TX->CCR, DMA_CCR_EN) == 0);

        const bool success = READ_BIT(DMAx_ISR, DMA_ISR_TCIF6);
        if (success) [[likely]] {
            // after successful transfer, no recovery is needed
            SPI_ASSERT("dust should have settled\n", READ_BIT(SPIx->SR, SPI_SR_FTLVL | SPI_SR_FRLVL | SPI_SR_BSY) == 0);
        } else {
            // error path
            SPI_ASSERT("must have been transfer error then\n", READ_BIT(DMAx_ISR, DMA_ISR_TEIF6 | DMA_ISR_TEIF7));
            while (LL_SPI_GetTxFIFOLevel(SPIx) != LL_SPI_TX_FIFO_EMPTY) {
                // wait for transmission
            }
            while (LL_SPI_IsActiveFlag_BSY(SPIx)) {
                // keep waiting
            }
            while (LL_SPI_GetRxFIFOLevel(SPIx) != LL_SPI_RX_FIFO_EMPTY) {
                (void)LL_SPI_ReceiveData8(SPIx);
            }
            LL_SPI_ClearFlag_OVR(SPIx);
        }

        // drive SCK pin after CS pin to ensure there is no false clock transition
        WRITE_REG(cs_port->BSRR, cs_pin);
        CLEAR_BIT(SPIx->CR1, SPI_CR1_SPE);

        return success;
    }

} // namespace

namespace accel {
    namespace {
        class SPIDevice : Uncopyable {
        public:
            [[nodiscard]] bool transmit_receive(Bytes tx, WritableBytes rx) {
                SPI_ASSERT("sizes must match\n", tx.size() == rx.size());
                SPI_ASSERT("config must not be changed in-flight\n", !LL_SPI_IsEnabled(SPIx));

                MODIFY_REG(SPIx->CR1, SPI_CR1_CPOL, LL_SPI_POLARITY_HIGH);
                return hal::spi::transmit_receive({
                    .tx_data = tx.data(),
                    .rx_data = rx.data(),
                    .size = tx.size(),
                    .cs_port = GPIOC,
                    .cs_pin = GPIO_PIN_6,
                });
            }
        };

        class AIS2IH final : public ::AIS2IH::Impl<SPIDevice> {
        public:
            using ::AIS2IH::Impl<SPIDevice>::Impl;
        };

        AIS2IH ais2ih;
    } // namespace

    [[nodiscard]] bool get_sample(Sample &sample) {
        ::AIS2IH::Sample ais2ih_sample;
        if (ais2ih.get_sample(ais2ih_sample)) [[likely]] {
            sample.x = ais2ih_sample.x;
            sample.y = ais2ih_sample.y;
            sample.z = ais2ih_sample.z;
            return true;
        } else {
            return false;
        }
    }

    /// Blocks until the accelerometer is detected and configured.
    /// The accelerometer is critical — the system cannot operate without it.
    void init() {
        while (!ais2ih.is_chip_detected()) {
            freertos::yield();
        }
        while (!ais2ih.setup()) {
            freertos::yield();
        }
    }
} // namespace accel

namespace loadcell {
    namespace {
        class SPIDevice : Uncopyable {
        public:
            [[nodiscard]] bool transmit_receive(Bytes tx, WritableBytes rx) {
                SPI_ASSERT("sizes must match\n", tx.size() == rx.size());
                SPI_ASSERT("config must not be changed in-flight\n", !LL_SPI_IsEnabled(SPIx));

                MODIFY_REG(SPIx->CR1, SPI_CR1_CPOL, LL_SPI_POLARITY_LOW);
                return hal::spi::transmit_receive({
                    .tx_data = tx.data(),
                    .rx_data = rx.data(),
                    .size = tx.size(),
                    .cs_port = GPIOA,
                    .cs_pin = GPIO_PIN_15,
                });
            }
        };

        class ADS131M02 final : public ::ADS131M02::Impl<SPIDevice> {
        public:
            using ADS131M02::Impl<SPIDevice>::Impl;

            void enable_adc_mco() const final {
                hal::clk::enable_adc_mco();
            }

            void disable_adc_mco() const final {
                hal::clk::disable_adc_mco();
            }

            void delay(uint32_t ms) const final {
                freertos::delay(ms);
            }
        };

        ADS131M02 ads131m02;
    } // namespace

    void init() {
        /// Blocks until the loadcell is detected and configured.
        /// The loadcell is critical — the system cannot operate without it.
        while (!ads131m02.init_comm()) {
            freertos::yield();
        }
    }

    std::optional<uint32_t> get_sample() {
        if (auto result = ads131m02.read_sample()) [[likely]] {
            return *result;
        } else {
            switch (result.error()) {
            case ::ADS131M02::ReadSampleError::transmit_failed:
            case ::ADS131M02::ReadSampleError::not_ready:
                break;
            case ::ADS131M02::ReadSampleError::crc_mismatch:
                hal::panic(indx_head::errors::FaultStatusMask::loadcell_crc_mismatch);
            }
            return std::nullopt;
        }
    }

} // namespace loadcell

void spi1_init() {
    LL_SPI_Disable(SPIx);
    LL_SPI_SetMode(SPIx, LL_SPI_MODE_MASTER);
    LL_SPI_SetTransferDirection(SPIx, LL_SPI_FULL_DUPLEX);
    LL_SPI_SetDataWidth(SPIx, LL_SPI_DATAWIDTH_8BIT);
    LL_SPI_SetClockPolarity(SPIx, LL_SPI_POLARITY_LOW);
    LL_SPI_SetClockPhase(SPIx, LL_SPI_PHASE_2EDGE);
    LL_SPI_SetNSSMode(SPIx, LL_SPI_NSS_SOFT);
    LL_SPI_SetBaudRatePrescaler(SPIx, LL_SPI_BAUDRATEPRESCALER_DIV8);
    LL_SPI_SetTransferBitOrder(SPIx, LL_SPI_MSB_FIRST);
    LL_SPI_DisableCRC(SPIx);
    LL_SPI_SetRxFIFOThreshold(SPIx, LL_SPI_RX_FIFO_TH_QUARTER);
    LL_SPI_EnableNSSPulseMgt(SPIx);
    SET_BIT(SPIx->CR2, SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);

    SPI_ASSERT("DMA clock must be enabled\n", READ_BIT(RCC->AHBENR, RCC_AHBENR_DMA1EN));
    constexpr uint32_t DMAMUX_REQ_SPI1_RX = 0x00000010;
    constexpr uint32_t DMAMUX_REQ_SPI1_TX = 0x00000011;
    WRITE_REG(DMAMUX_CHAN_RX->CCR, DMAMUX_REQ_SPI1_RX);
    WRITE_REG(DMAMUX_CHAN_TX->CCR, DMAMUX_REQ_SPI1_TX);
    SPI_ASSERT("DMA_CCRx must be at reset value (RX)\n", READ_REG(DMA_CHAN_RX->CCR) == 0);
    SPI_ASSERT("DMA_CCRx must be at reset value (TX)\n", READ_REG(DMA_CHAN_TX->CCR) == 0);
    WRITE_REG(DMA_CHAN_RX->CPAR, DMA_CPARx);
    WRITE_REG(DMA_CHAN_TX->CPAR, DMA_CPARx);
}

void dma_isr() {
    // read register once and store it for further inspection
    const uint32_t local_DMAx_ISR = READ_REG(DMAx->ISR);
    // no matter which event happens, we handle it the same
    if (READ_BIT(local_DMAx_ISR, DMA_ISR_TEIF6 | DMA_ISR_TEIF7 | DMA_ISR_TCIF6)) {
        DMAx_ISR = local_DMAx_ISR;
        // disable channels
        WRITE_REG(DMA_CHAN_TX->CCR, 0);
        WRITE_REG(DMA_CHAN_RX->CCR, 0);
        // clear flags to prevent ISR from firing again
        WRITE_REG(DMAx->IFCR, DMA_IFCR_CGIF6 | DMA_IFCR_CGIF7);
        // wakeup task
        semaphore.release_from_isr();
    }
}

void init_comm() {
    accel::init();
    loadcell::init();
}

} // namespace hal::spi
