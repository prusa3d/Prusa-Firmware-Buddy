/// @file
#pragma once

#include <ac_controller/faults.hpp>
#include <array>
#include <cstdint>
#include <optional>

namespace ac_controller {

/// Type used for reporting current status of AC Controller.
struct Status {
    float mcu_temp; ///< MCU temperature (Celsius)
    float bed_temp; ///< bed temperature (Celsius)
    float bed_voltage; ///< bed voltage (Volt)
    std::array<uint16_t, 2> bed_fan_rpm; ///< bed fan RPM (RPM)
    uint16_t psu_fan_rpm; ///< PSU fan RPM (RPM)
    ac_controller::Faults faults;
};

/// Color
struct ColorRGBW {
    uint8_t r; ///< red component (0-255)
    uint8_t g; ///< green component (0-255)
    uint8_t b; ///< blue component (0-255)
    uint8_t w; ///< white component (0-255)

    auto operator<=>(const ColorRGBW &) const = default;
};

/// Type used for setting desired config of AC Controller.
struct Config {
    std::optional<float> bed_target_temp; ///< bed target temperature (Celsius)
    std::optional<uint8_t> bed_fan_pwm; ///< bed fan PWM (0-255) (shared by both bed fans)
    std::optional<uint8_t> psu_fan_pwm; ///< PSU fan PWM (0-255)

    auto operator<=>(const Config &) const = default;
};

enum class AnimationType : uint16_t {
    OFF = 0,
    STATIC_COLOR = 1,
    PROGRESS_PERCENT = 2,
    _last = PROGRESS_PERCENT
};

struct LedConfig {
    std::optional<ColorRGBW> color; ///< LED color
    std::optional<uint8_t> progress_percent; /// Progress percentage to display on bed led strip (0-100)
    std::optional<AnimationType> animation_type; /// Animation type

    bool operator<=>(const LedConfig &other) const = default;
};
} // namespace ac_controller
