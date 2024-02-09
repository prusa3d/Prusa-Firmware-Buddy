#include "quick_tmc_spi.hpp"

#include <trinamic.h>
#include <TMCStepper.h>

#include <device/peripherals.h>
#include <hwio_pindef.h>

using namespace phase_stepping;
using namespace phase_stepping::spi;
using namespace phase_stepping::opts;
using namespace buddy::hw;

static uint8_t xdirect_comm_buffer[5] = { 0, 0, 0, 0, 0 };
static int active_axis = 0;
static const std::array<OutputPin, 3> cs_pins = { { xCs, yCs, zCs } };

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
    xdirect_comm_buffer[0] = 0x80 | XDIRECT_t::address;
    xdirect_comm_buffer[1] = (raw >> 24) & 0xFF;
    xdirect_comm_buffer[2] = (raw >> 16) & 0xFF;
    xdirect_comm_buffer[3] = (raw >> 8) & 0xFF;
    xdirect_comm_buffer[4] = (raw >> 0) & 0xFF;
}

bool phase_stepping::spi::initialize_transaction() {
    return !tmc_serial_lock_requested_by_task() && tmc_serial_lock_acquire_isr();
}

void phase_stepping::spi::set_xdirect(int axis, const CoilCurrents &currents) {
    // You might be wondering why we don't set the state of peripheral in HAL
    // handle: it is a wasted 2 µs ns and since we hold a lock, there cannot be
    // a race.
    active_axis = axis;
    setup_xdirect_buffer(currents.a, currents.b);
    cs_pins[active_axis].write(Pin::State::low);

    // Setup SPI
    PHSTEP_TMC_SPI->CR2 = (PHSTEP_TMC_SPI->CR2
                              & ~(SPI_CR2_RXNEIE_Msk | SPI_CR2_ERRIE_Msk | SPI_CR2_TXEIE_Msk))
        | SPI_CR2_TXDMAEN_Msk;
    PHSTEP_TMC_SPI->CR1 |= SPI_CR1_SPE;

    // Setup DMA
    assert(!busy());
    PHSTEP_TMC_DMA->NDTR = 5;
    PHSTEP_TMC_DMA->PAR = reinterpret_cast<uint32_t>(&PHSTEP_TMC_SPI->DR);
    PHSTEP_TMC_DMA->M0AR = reinterpret_cast<uint32_t>(&xdirect_comm_buffer[0]);
    PHSTEP_TMC_DMA_REGS->IFCR = 0b111111 << PHSTEP_TMC_DMA_REGS_OFFSET;
    PHSTEP_TMC_DMA->CR = (PHSTEP_TMC_DMA->CR
                             & ~(DMA_SxCR_DBM_Msk | DMA_SxCR_DMEIE_Msk | DMA_SxCR_TEIE_Msk | DMA_SxCR_HTIE_Msk | DMA_SxCR_TCIE_Msk))
        | DMA_SxCR_EN_Msk;
}

void phase_stepping::spi::finish_transmission() {
    if (!tmc_serial_lock_held_by_isr()) {
        return;
    }

    cs_pins[active_axis].write(Pin::State::high);
    tmc_serial_lock_release_isr();
}

bool phase_stepping::spi::busy() {
    return (PHSTEP_TMC_DMA->NDTR > 0) && (PHSTEP_TMC_DMA->CR & DMA_SxCR_EN_Msk);
}
