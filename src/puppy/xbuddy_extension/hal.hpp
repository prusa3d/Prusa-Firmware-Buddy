/// @file
#pragma once

#include "option/extension_variant.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <xbuddy_extension/shared_enums.hpp>

namespace hal {

using DutyCycle = uint8_t;

/**
 * Initialize hardware abstraction layer module.
 * This must be called while still in privileged mode,
 * because it needs to setup interrupts.
 */
void init();

/// Called once in the hal_task
void setup();

/**
 * Enter infinite loop.
 */
[[noreturn]] void panic();

/**
 * Step the HAL subsystem.
 * This blocks and must be called periodically.
 */
void step();

// Each peripheral gets its own namespace

namespace fan1 {
    void set_pwm(DutyCycle duty_cycle);
    uint32_t get_rpm();
} // namespace fan1

namespace fan2 {
    void set_pwm(DutyCycle duty_cycle);
    uint32_t get_rpm();
} // namespace fan2

namespace fan3 {
    void set_pwm(DutyCycle duty_cycle);
    uint32_t get_rpm();
} // namespace fan3

#if EXTENSION_IS_XL_CAN()
/// Fan 5 V power switch, XL-CAN bridge PCB only (PA6 EN / PB13 fault drive a
/// TPS2041C; on xBE those pins serve other roles and the fan rail has no
/// switch). This is the "fully disable the fan" mechanism — PWM 0 alone
/// leaves a 4-wire fan powered.
namespace fan_power {
    /// Parks the gate off (the on-board pull-up holds it off through MCU reset).
    void init();
    /// true powers the fan 5 V rail (EN line is active low at the TPS2041C).
    void enable_pin_set(bool enabled);
    /// true = the TPS2041C reports overcurrent/overtemperature (active-low
    /// open-drain fault input, board pull-up).
    bool fault_pin_get();
} // namespace fan_power
#endif

#if PA6_PIN_DRIVES_W_LED()
namespace w_led {
    void set_pwm(DutyCycle duty_cycle);
    /**
     * Frequency of the PWM cycle
     *
     * In Hz.
     *
     * 0 means "default" selected by us.
     */
    void set_frequency(uint16_t freq);
} // namespace w_led
#endif

namespace rgbw_led {
    void set_r_pwm(DutyCycle duty_cycle);
    void set_g_pwm(DutyCycle duty_cycle);
    void set_b_pwm(DutyCycle duty_cycle);
    void set_w_pwm(DutyCycle duty_cycle);
} // namespace rgbw_led

namespace temperature {
    uint32_t get_raw();
}

namespace filament_sensor {
    using State = xbuddy_extension::FilamentSensorState;

    /// Single GPIO sensor (PA5 on standard, PA9 on iX)
    State get_gpio();

    /// TMP1826 multi-tool sensors (PC14/EXT connector)
    State get_ext(uint8_t index);
} // namespace filament_sensor

} // namespace hal
