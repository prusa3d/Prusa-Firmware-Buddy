#pragma once

#include <stm32c0xx_hal.h>

#include <indx_head/errors.hpp>
#include <indx_head/leds.hpp>
#include <freertos/binary_semaphore.hpp>
#include <tpis/tpis.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utils/byte_utils.hpp>
#include <utility>

namespace hal {

/// default ISR priority, used by ISRs that don't need specific ISR priority
static constexpr uint8_t ISR_PRIORITY_DEFAULT = 2;

void init();
void config_adc_default();
void config_adc_induction_heater();

indx_head::errors::FaultStatusMask get_fault_status();
void set_fault_status(indx_head::errors::FaultStatusMask);
void clear_fault_status(indx_head::errors::FaultStatusMask);
void __attribute__((noreturn)) panic(indx_head::errors::FaultStatusMask);

void set_ind_heater_pwr(bool enabled);

void set_safe_state();

namespace rs485 {
    void init();
    WritableBytes maybe_transmit_and_then_receive(WritableBytes data);
} // namespace rs485

struct CheckedTemperatureReading {
    tpis::TemperatureReading temps;
    bool valid;
};

namespace i2c {
    // Does everything to prepare and initialize all the I2C peripherals
    void init_comm();

    /// Returns ambient and object temperatures
    CheckedTemperatureReading read_tpis_temperature();
    void set_led_pwm(uint8_t r, uint8_t g, uint8_t b);
    void set_led_fade(uint16_t interval_ms);

    void trigger_delayed_request();
} // namespace i2c

namespace tim {
    void set_printfan_pwm(uint8_t pwm);
    void reset_printfan_rpm_counter();
    uint16_t get_printfan_rpm_counter();

    void set_heatbreak_fan_pwm(uint8_t pwm);
    void reset_heatbreak_fan_rpm_counter();
    uint16_t get_heatbreak_fan_rpm_counter();
} // namespace tim

namespace adc {
    enum class Channel : uint8_t {
        board_temp,
        induction_feadback,
        mcu_temp,
        vref_int,
        input_voltage,
        heater_current,
        _cnt
    };

    /// Returs raw adc value for given @p channel
    uint16_t get_raw(Channel channel);
    /// Calcules MCU temp in degC
    int16_t get_mcu_temp();
    /// Calculates board temp in degC
    int16_t get_board_temp();
    /// Calculates input volatage in mV
    uint16_t get_input_voltage_mV();
    /// Calculates heater current in mA
    uint16_t get_heater_current_mA();

    namespace impl {
        alignas(uint32_t) extern std::array<uint16_t, std::to_underlying(Channel::_cnt)> buffer;
    }
} // namespace adc

namespace clk {
    /// Enables MCO output of ADC clock
    void enable_adc_mco();
    /// Disables MCO output of ADC clock
    void disable_adc_mco();
} // namespace clk

namespace peripherals {
    extern ADC_HandleTypeDef hadc1;
    extern freertos::BinarySemaphore adc_semaphore;
} // namespace peripherals

} // namespace hal
