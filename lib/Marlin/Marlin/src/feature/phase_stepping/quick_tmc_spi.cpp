#include "quick_tmc_spi.hpp"

#include <trinamic.h>
#include <TMCStepper.h>

#include <device/peripherals.h>
#include <hwio_pindef.h>

using namespace phase_stepping;
using namespace phase_stepping::spi;
using namespace buddy::hw;

static uint8_t xdirect_comm_buffer[5] = { 0x80 | XDIRECT_t::address, 0, 0, 0, 0 };
static int active_axis = 0;
static const std::array<OutputPin, 3> cs_pins = { { xCs, yCs, zCs } };

// You might be wondering why we use hard-coded constants and we don't read
// them from handles: it is wasted 500 ns - that is 2 % of CPU load
#define TMC_SPI             SPI3
#define TMC_DMA             DMA1_Stream5
#define TMC_DMA_REGS        reinterpret_cast<DMA_Base_Registers *>((((uint32_t)TMC_DMA & (uint32_t)(~0x3FFU)) + 4U))
#define TMC_DMA_REGS_OFFSET 6U

struct DMA_Base_Registers {
    __IO uint32_t ISR; /*!< DMA interrupt status register */
    __IO uint32_t Reserved0;
    __IO uint32_t IFCR; /*!< DMA interrupt flag clear register */
};

static void setup_xdirect_buffer(int current_a, int current_b) {
    XDIRECT_t reg;
    // Swapping coils isn't a mistake - TMC in Xdirect mode swaps coils
    reg.coil_A = current_b;
    reg.coil_B = current_a;
    uint32_t raw = reg.sr;
    xdirect_comm_buffer[1] = (raw >> 24) & 0xFF;
    xdirect_comm_buffer[2] = (raw >> 16) & 0xFF;
    xdirect_comm_buffer[3] = (raw >> 8) & 0xFF;
    xdirect_comm_buffer[4] = (raw >> 0) & 0xFF;
}

HAL_StatusTypeDef phase_stepping::spi::set_xdirect(int axis, int current_a, int current_b) {
    if (!tmc_serial_lock_acquire_isr()) {
        return HAL_BUSY;
    }

    // You might be wondering why we don't set the state of peripheral in HAL
    // handle: it is a wasted 2 µs ns and since we hold a lock, there cannot be
    // a race.

    active_axis = axis;
    setup_xdirect_buffer(current_a, current_b);
    cs_pins[active_axis].write(Pin::State::low);

    // Setup SPI
    TMC_SPI->CR2 = (TMC_SPI->CR2
                       & ~(SPI_CR2_RXNEIE_Msk | SPI_CR2_ERRIE_Msk | SPI_CR2_TXEIE_Msk))
        | SPI_CR2_TXDMAEN_Msk;
    TMC_SPI->CR1 |= SPI_CR1_SPE;

    // Setup DMA
    TMC_DMA->NDTR = 5;
    TMC_DMA->PAR = reinterpret_cast<uint32_t>(&TMC_SPI->DR);
    TMC_DMA->M0AR = reinterpret_cast<uint32_t>(&xdirect_comm_buffer[0]);
    TMC_DMA_REGS->IFCR = 0b111111 << TMC_DMA_REGS_OFFSET;
    TMC_DMA->CR = (TMC_DMA->CR
                      & ~(DMA_SxCR_DBM_Msk | DMA_SxCR_DMEIE_Msk | DMA_SxCR_TEIE_Msk | DMA_SxCR_HTIE_Msk | DMA_SxCR_TCIE_Msk))
        | DMA_SxCR_EN_Msk;

    return HAL_OK;
}

void phase_stepping::spi::finish_transmission() {
    if (!tmc_serial_lock_held_by_isr()) {
        return;
    }

    cs_pins[active_axis].write(Pin::State::high);
    tmc_serial_lock_release_isr();
}
