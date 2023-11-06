#include "burst_stepper.hpp"
#include "common.hpp"
#include <hwio.h>
#include <array>
#include <device/peripherals.h>
#include "otp.hpp"

using namespace phase_stepping;
using namespace phase_stepping::opts;
using namespace burst_stepping;
using namespace buddy::hw;

// You might be wondering why we use hard-coded constants and we don't read
// them from handles: it is wasted 500 ns - that is 2 % of CPU load
#define BURST_DMA             DMA2_Stream1
#define BURST_DMA_REGS        reinterpret_cast<DMA_Base_Registers *>((((uint32_t)BURST_DMA & (uint32_t)(~0x3FFU))))
#define BURST_DMA_REGS_OFFSET 6u

struct DMA_Base_Registers {
    __IO uint32_t ISR; /*!< DMA interrupt status register */
    __IO uint32_t Reserved0;
    __IO uint32_t IFCR; /*!< DMA interrupt flag clear register */
};

// GPIO event buffer that can be played via DMA. Supports fast clearing.
template <int SIZE>
class GpioEventBuffer {
public:
    GpioEventBuffer() {
        _buffer.fill(0);
        _event_positions.fill(0);
    }

    void clear() {
        if (_event_count < SIZE) {
            // It is worth it to erase the array sparely
            for (int i = 0; i != _event_count; i++) {
                _buffer[_event_positions[i]] = 0;
            }
        } else {
            // It is not worth it, do bulk erase
            _buffer.fill(0);
        }
        _event_count = 0;
    };

    void add_event(int idx, uint32_t event_mask) {
        assert(idx < SIZE);
        _buffer[idx] |= event_mask;

        if (_event_count < SIZE) {
            _event_positions[_event_count] = idx;
            _event_count++;
        }
    }

    static int size() {
        return SIZE;
    }

    const uint32_t *dma_buffer() const {
        return _buffer.data();
    }

private:
    std::array<uint32_t, SIZE> _buffer {};
    // GPIO events are sparse. Therefore, instead of clearing the whole buffer,
    // we mark the event positions.
    int _event_count = 0;
    std::array<uint16_t, SIZE> _event_positions {};
};

template <typename T>
using PerAxisArray = std::array<T, SUPPORTED_AXIS_COUNT>;

// Buddy pin abstraction for dir signals
static constexpr PerAxisArray<OutputPin> dir_signals = { { xDir, yDir } };

// Bitmasks for step pins on the port
static GPIO_TypeDef *step_gpio_port = nullptr;
static PerAxisArray<uint32_t> step_masks;

// Module items
static PerAxisArray<bool> axis_direction;
static PerAxisArray<bool> axis_step_state;
static PerAxisArray<bool> axis_was_set;
static std::array<GpioEventBuffer<GPIO_BUFFER_SIZE>, 2> porta_event_buffers;
static GpioEventBuffer<GPIO_BUFFER_SIZE>
    *setup_buffer = &porta_event_buffers[0],
    *fire_buffer = &porta_event_buffers[1];

static void setup_pinout_1() {
    step_gpio_port = GPIOA;
    step_masks = { { 1 << 0, 1 << 3 } };
}

static void setup_pinout_2() {
    step_gpio_port = GPIOD;
    step_masks = { { 1 << 7, 1 << 5 } };
}

void burst_stepping::init() {
    auto otp_bom_id = otp_get_bom_id();
    if (!otp_bom_id.has_value()) {
        bsod("Unable to determine board BOM ID");
    }
    auto board_bom_id = *otp_bom_id;

    if (board_bom_id >= 9 || board_bom_id == 4) {
        setup_pinout_2();
    } else {
        setup_pinout_1();
    }

    HAL_TIM_Base_Start(&TIM_HANDLE_FOR(burst_stepping));
    __HAL_TIM_ENABLE_DMA(&TIM_HANDLE_FOR(burst_stepping), TIM_DMA_UPDATE);
}

__attribute__((optimize("-Ofast"))) void burst_stepping::set_phase_diff(AxisEnum axis, int diff) {
    if (axis > SUPPORTED_AXIS_COUNT) {
        bsod("Unsupported axis");
    }

    if (diff < 0) {
        diff = -diff;
        axis_direction[axis] = false;
    } else {
        axis_direction[axis] = true;
    }

    if (diff > GPIO_BUFFER_SIZE) {
        bsod("Axis speed over limit");
    }

    if (diff == 0) {
        return;
    }

    // We use fixed 16.16 number to find the transition points
    std::size_t udiff = diff;
    std::size_t spacing = (GPIO_BUFFER_SIZE << 16) / diff;
    bool current_state = axis_step_state[axis];
    for (std::size_t i = 0; i != udiff; i++) {
        current_state = !current_state;
        // The added constant rounds the index.
        std::size_t idx = (spacing * i + (1ul << 15)) >> 16;
        if (current_state) {
            setup_buffer->add_event(idx, step_masks[axis]);
        } else {
            setup_buffer->add_event(idx, step_masks[axis] << 16);
        }
    }
    axis_step_state[axis] = current_state;
    axis_was_set[axis] = true;
}

__attribute__((optimize("-Ofast"))) static void setup_and_fire_dma() {
    BURST_DMA->CR = BURST_DMA->CR & (~DMA_SxCR_EN_Msk);
    BURST_DMA->NDTR = fire_buffer->size();
    BURST_DMA->PAR = reinterpret_cast<uint32_t>(&step_gpio_port->BSRR);
    BURST_DMA->M0AR = reinterpret_cast<uint32_t>(fire_buffer->dma_buffer());
    BURST_DMA_REGS->IFCR = 0b111111 << BURST_DMA_REGS_OFFSET;
    __HAL_TIM_SET_COUNTER(&TIM_HANDLE_FOR(burst_stepping), 0);
    BURST_DMA->CR = (BURST_DMA->CR
                        & ~(DMA_SxCR_DBM_Msk | DMA_SxCR_DMEIE_Msk | DMA_SxCR_TEIE_Msk | DMA_SxCR_HTIE_Msk | DMA_SxCR_TCIE_Msk))
        | DMA_SxCR_EN_Msk;
}

__attribute__((optimize("-Ofast"))) void burst_stepping::fire() {
    for (std::size_t i = 0; i != axis_direction.size(); i++) {
        if (!axis_was_set[i]) {
            continue;
        }
        axis_was_set[i] = false;
        if (axis_direction[i]) {
            dir_signals[i].write(Pin::State::low);
        } else {
            dir_signals[i].write(Pin::State::high);
        }
    }
    delay_ns_precise<100>(); //...to settle DIR pins
    std::swap(setup_buffer, fire_buffer);
    setup_and_fire_dma();
    setup_buffer->clear();
}
