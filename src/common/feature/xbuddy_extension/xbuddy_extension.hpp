#pragma once

#include "cooling.hpp"

#include <optional>
#include <span>

#include <freertos/mutex.hpp>
#include <leds/color.hpp>
#include <temperature.hpp>

#include <xbuddy_extension_shared/xbuddy_extension_shared_enums.hpp>

namespace buddy {

/// Thread-safe API, can be read/written to from any thread
class XBuddyExtension {
public: // General things, status
    XBuddyExtension();

    enum class Status {
        disabled,
        not_connected,
        ready,
    };

    using FilamentSensorState = xbuddy_extension_shared::FilamentSensorState;

    Status status() const;

    void step();

public: // Fans
    /// \returns measured RPM of the fan1 (chamber cooling)
    std::optional<uint16_t> fan1_rpm() const;

    /// \returns measured RPM of the fan2 (chamber cooling)
    std::optional<uint16_t> fan2_rpm() const;

    /// \returns shared target PWM for fan1/fan2
    uint8_t fan1_fan2_pwm() const;

    /// Sets PWM for fan 1 and fan 2 (chamber cooling fans; 0-255)
    /// The fans have a single shared PWM line, so they can only be set both at once
    /// Disables \p has_fan1_fan2_auto_control control
    void set_fan1_fan2_pwm(uint8_t pwm);

    /// \returns whether the fan1 & fan2 are in auto control mode (or values set explicitly by set_XX_pwm)
    bool has_fan1_fan2_auto_control() const;

    /// See \p has_fan1_fan2_auto_control
    void set_fan1_fan2_auto_control();

    /// \returns measured RPM of the fan3 (in-chamber filtration)
    std::optional<uint16_t> fan3_rpm() const;

    /// \returns target PWM for fan3
    uint8_t fan3_pwm() const;

    /// Sets PRM for fan 3 (in-chamber filtration, 0-255)
    void set_fan3_pwm(uint8_t pwm);

    /// A convenience function returning a structure of data to be used in the Connect interface
    /// The key idea here is to avoid locking the internal mutex for every member while providing a consistent state of values.
    struct FanState {
        uint16_t fan1rpm, fan2rpm;
        uint8_t fan12pct;
        bool fan12autocontrol;
    };

    FanState get_fan12_state() const;

public: // LEDs
    /// \returns color set for the bed LED strip
    leds::ColorRGBW bed_leds_color() const;

    /// Sets PWM for the led strip that is under the bed
    void set_bed_leds_color(leds::ColorRGBW set);

    /// \returns PWM for the white led strip thas is in the chamber (0-255)
    uint8_t chamber_leds_pwm();

    /// Sets PWM for the white led strip thas is in the chamber (0-255)
    void set_chamber_leds_pwm(uint8_t set);

    /// @returns percentage 0-100% converted from PWM value (0-max_pwm)
    /// @note in the future, non-linear mapping between intensity pct and PWM shall be implemented here
    static constexpr uint8_t led_pwm2pct(uint8_t pwm) {
        return static_cast<uint8_t>(((uint16_t)pwm) * 100U / 255U);
    }

    /// @returns PWM value (0-max_pwm) from percentage 0-100%
    /// @note in the future, non-linear mapping between intensity pct and PWM shall be implemented here
    static constexpr uint8_t led_pct2pwm(uint8_t pct) {
        return static_cast<uint8_t>(((uint16_t)pct) * 255U / 100U);
    }

public: // Other
    /// \returns chamber temperature measured through the thermistor connected to the board, in degrees Celsius
    std::optional<Temperature> chamber_temperature();

    /// \returns state of the filament sensor
    std::optional<FilamentSensorState> filament_sensor();

private:
    mutable freertos::Mutex mutex_;

    leds::ColorRGBW bed_leds_color_;

    FanCooling chamber_cooling;

    uint8_t fan3_pwm_ = 0;
};

XBuddyExtension &xbuddy_extension();

} // namespace buddy
