/// @file
#include "hal_ext_fs.hpp"
#include "hal.hpp"

#include <atomic>

#include <stm32h5xx_hal.h>

#include <filters/debouncer.hpp>
#include <utils/array_extensions.hpp>
#include <onewire_master/onewire_master.hpp>
#include <freertos/timing.hpp>
#include <crc/crc.hpp>
#include <bsod/bsod.h>

namespace {

using FSState = hal::filament_sensor::State;

const auto BUS_PORT = GPIOC;
constexpr uint32_t BUS_PIN = GPIO_PIN_14;

constexpr OneWireMaster::Timing timing {
    .reset_drive = 480,
    .reset_settle_before_presence_sample = 70,
    .reset_settle_after_presence_sample = 410,
    .write_bit_1_drive = 6,
    .write_bit_1_settle = 64,
    .write_bit_0_drive = 60,
    .write_bit_0_settle = 10,
    .read_bit_drive = 6,
    .read_bit_settle_before_sample = 9,
    .read_bit_settle_after_sample = 55,
};

/// Selects all devices on the bus
constexpr std::byte CMD_SKIP_ADDR { 0xCC };

/// Selects a device based on flex addr
constexpr std::byte CMD_FLEX_ADDR { 0x0F };

/// Configures the device
constexpr std::byte CMD_WRITE_SCRATCHPAD_1 { 0x4E };

/// Reads GPIO from the device
constexpr std::byte CMD_GPIO_READ { 0xF5 };

std::array<std::byte, 2> gpio_read_rx; // GPIO byte + CRC = 2 bytes

std::array gpio_read_tx {
    CMD_FLEX_ADDR,
    std::byte { 0 }, // Short addr, filled before each transfer
    CMD_GPIO_READ,
};

OneWireMaster onewire { timing };

static constexpr uint8_t fsensors_per_device = 4;
static constexpr uint8_t device_count = 2;
static_assert(fsensors_per_device * device_count == xbuddy_extension::ext_filament_sensor_count);

/// Per-filament sensor data
struct FilamentSensorData {
    Debouncer<FSState> debouncer { FSState::uninitialized, 3 };

    /// FS are read from whatever thread, so keep them in separate atomics
    std::atomic<FSState> state { FSState::uninitialized };
};

std::array<FilamentSensorData, xbuddy_extension::ext_filament_sensor_count> fs_data;

/// Short addresses that should automatically get assigned to the devices
/// During the initial setup() broadcast
static constexpr std::array<std::byte, device_count> device_short_addresses {
    std::byte { 0x0 },
    std::byte { 0xF },
};
static_assert(device_count == 2);

/// Currently scanned device, iterated after each scan
uint8_t current_device_ix = 0;

/// Phase of the step FSM
enum class StepPhase : uint8_t {
    issue_read,
    process_read,
};

StepPhase step_phase = StepPhase::issue_read;

/// If delay_us is not provided, start immediately
/// 0 does not work, we need to default to 1
void timer_start(uint16_t delay_us = 1) {
    TIM6->CNT = 0;
    TIM6->ARR = delay_us;
    TIM6->SR = 0; // Clear any pending interrupt
    TIM6->DIER = TIM_DIER_UIE; // Enable update interrupt
    TIM6->CR1 = TIM_CR1_CEN | TIM_CR1_OPM; // Start in one-pulse mode
}

} // namespace

FSState hal::filament_sensor::get_ext(uint8_t index) {
    debug_assert(index < xbuddy_extension::ext_filament_sensor_count);
    return fs_data[index].state.load();
}

void hal::ext_fs::setup() {
    // Broadcast to all devices to set their flex address according to the presence of a resistor on the board
    // (left and right fsensors have different BOMs)
    // After this, we will be addressing the devices solely by their flex address
    static_assert(device_count == 2);
    static constexpr std::array flex_addr_broadcast {
        CMD_SKIP_ADDR,
        CMD_WRITE_SCRATCHPAD_1,

        // Device configuration register 1
        // Related to temp and ADC readouts. We don't care about these, use default values.
        std::byte { 0b01110000 },

        // Device configuration register 2, more interesting
        // Bits 5-6: FLEX_ADDR_MODE. 0b10 sets it to resistor decode
        // The rest is default.
        std::byte { 0b11000000 },

        std::byte { 0x00 }, // SHORT_ADDR - overwritten by resistor decode
        std::byte { 0x00 }, // ALERT_LOW_L - default values, we don't care
        std::byte { 0x00 }, // ALERT_LOW_H - default values, we don't care
        std::byte { 0xF0 }, // ALERT_HIGH_L - default values, we don't care
        std::byte { 0x07 }, // ALERT_HIGH_H - default values, we don't care
        std::byte { 0x00 }, // OFFSET_L - default values, we don't care
        std::byte { 0x00 }, // OFFSET_H - default values, we don't care
    };

    onewire.start_transfer(flex_addr_broadcast, {});
    timer_start();
    while (onewire.is_active()) {
        freertos::delay(1);
    }
}

void hal::ext_fs::step() {
    if (onewire.is_active()) {
        // Wait for the onewire transaction to finish
        return;
    }

    switch (step_phase) {

    case StepPhase::issue_read: {
        // Issue read from the device
        gpio_read_tx[1] = device_short_addresses[current_device_ix];
        onewire.start_transfer(gpio_read_tx, gpio_read_rx);
        timer_start();
        step_phase = StepPhase::process_read;
        break;
    }

    case StepPhase::process_read: {
        // Validate CRC
        Crc8OneWire crc;
        crc.update(gpio_read_rx);
        const bool is_response_valid = onewire.presence_detected() && (crc.get() == 0);
        const std::byte gpio_data = gpio_read_rx[0];

        for (uint8_t i = 0; i < fsensors_per_device; i++) {
            // GPIO bits are wired in descending order on the PCB: bit 0 = last tool in group, bit 3 = first
            auto &fs = fs_data[current_device_ix * fsensors_per_device + (fsensors_per_device - 1 - i)];

            FSState state = FSState::disconnected;
            if (is_response_valid) {
                state = bool((gpio_data >> i) & std::byte { 1 }) ? FSState::has_filament : FSState::no_filament;
            }

            // Debounce
            fs.debouncer.push(state);

            // And store to the atomic to be read by other threads
            fs.state.store(fs.debouncer.value());
        }

        current_device_ix = (current_device_ix + 1) % device_count;
        step_phase = StepPhase::issue_read;
        break;
    }
    }
}

void hal::ext_fs::init() {
    // Configure TIM6 for 1µs ticks (used as one-pulse delay generator)
    __HAL_RCC_TIM6_CLK_ENABLE();

    TIM6->CR1 = 0;
    // APB1 timers get 2x clock when APB1 prescaler > 1
    TIM6->PSC = (HAL_RCC_GetPCLK1Freq() * 2 / 1000000) - 1; // 1MHz (1µs per tick)
    TIM6->ARR = 0xFFFF;
    TIM6->EGR = TIM_EGR_UG; // Load prescaler immediately

    // Configure PC14 as open-drain output with internal pull-up (1-Wire bus)
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Set the pin to high (= open drain)
    BUS_PORT->BSRR = BUS_PIN;

    static constexpr GPIO_InitTypeDef gpio {
        .Pin = BUS_PIN,
        .Mode = GPIO_MODE_OUTPUT_OD,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = 0,
    };
    HAL_GPIO_Init(BUS_PORT, &gpio);

    HAL_NVIC_SetPriority(TIM6_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);
}

namespace {

void timer_isr() {
    const auto r = onewire.step(OneWireMaster::StepArgs {
        .bus_is_low = (BUS_PORT->IDR & BUS_PIN) == 0,
    });

    // Reset bit - drive low; set bit - release to pull up thanks to open drain
    BUS_PORT->BSRR = r.drive_bus_low ? (BUS_PIN << 16) : BUS_PIN;

    if (!r.finished) {
        timer_start(r.next_step_delay_us);
    }
}

} // namespace

extern "C" void TIM6_IRQHandler() {
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR = ~TIM_SR_UIF; // Clear interrupt flag
        timer_isr();
    }
}
