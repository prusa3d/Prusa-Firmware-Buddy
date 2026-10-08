/// @file
#pragma once

#include <array>
#include <cstdint>

/// This file defines MODBUS register files, to be shared between master and slave.
/// Resist the temptation to make this type-safe in any way! This is only used for
/// memory layout and should consist of 16-bit values, arrays and structures of such.

namespace ac_controller::modbus {

/// MODBUS register file for reporting current status of ac_controller to motherboard.
struct Status {
    static constexpr uint16_t address = 0x8000;

    uint16_t mcu_temp; ///< MCU temperature (decidegree Celsius)
    uint16_t bed_temp; ///< bed temperature (decidegree Celsius)
    uint16_t bed_voltage; ///< bed voltage (deci Volt)
    std::array<uint16_t, 2> bed_fan_rpm; ///< bed fan RPM (RPM)
    uint16_t psu_fan_rpm; ///< PSU fan RPM (RPM)
    uint16_t faults_lo; ///< ac_controller::Faults (lower 16 bits)
    uint16_t faults_hi; ///< ac_controller::Faults (upper 16 bits)
};

/// MODBUS register file for setting desired config of ac_controller from motherboard.
struct Config {
    static constexpr uint16_t address = 0x9000;

    uint16_t bed_target_temp; ///< bed target temperature (decidegree Celsius) (0 = not set)
    uint16_t bed_fan_pwm; ///< bed fan PWM (0-255) (shared by both bed fans)
    uint16_t psu_fan_pwm; ///< PSU fan PWM (0-255)
};

struct LedConfig {
    static constexpr uint16_t address = 0x9100;
    uint16_t animation_type; /// LED animation type (0 = off, 1 = static color, 2 = progress percent)
    uint16_t led_r; ///< LED red component (0-255)
    uint16_t led_g; ///< LED green component (0-255)
    uint16_t led_b; ///< LED blue component (0-255)
    uint16_t led_w; ///< LED white component (0-255)
    uint16_t progress_percent; /// Progress percentage to display on bed led strip (0-100) (0 = not set)
};

} // namespace ac_controller::modbus
