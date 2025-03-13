#pragma once

#include "cooling.hpp"

#include <optional>

#include <enum_array.hpp>
#include <freertos/mutex.hpp>
#include <leds/color.hpp>
#include <temperature.hpp>
#include <pwm_utils.hpp>

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

    using Fan = xbuddy_extension_shared::Fan;
    using FilamentSensorState = xbuddy_extension_shared::FilamentSensorState;
    using FanRPM = uint16_t;
    using FanPWM = PWM255;
    using FanPWMOrAuto = PWM255OrAuto;

    Status status() const;

    void step();

public: // Fans
    /// \returns measured RPM of the specified fan
    std::optional<FanRPM> fan_rpm(Fan fan) const;

    /// \returns False on unexpected fan behaviour (positive PWM but 0 RPM)
    bool is_fan_ok(const Fan fan) const;

    /// \returns shared target PWM of the specified fan
    FanPWMOrAuto fan_target_pwm(Fan fan) const;

    /// \returns actual PWM the fans are controlled to
    FanPWM fan_actual_pwm(Fan fan) const;

    /// Sets target PWM for the given fan. The PWM can be overriden by some emergency events
    /// * Please note than PWM control for the cooling fans is shared (so calling this with Fan::cooling_fan_1 does the same as with Fan::cooling_fan_2)
    void set_fan_target_pwm(Fan fan, FanPWMOrAuto target);

    /// A convenience function returning a structure of data to be used in the Connect interface
    /// The key idea here is to avoid locking the internal mutex for every member while providing a consistent state of values.
    struct FanState {
        uint16_t fan1rpm, fan2rpm;
        FanPWMOrAuto fan1_fan2_target_pwm;
    };

    FanState get_fan12_state() const;

    /// \returns whether the fan 3 is connected/used and thus whether we should consider it in sensor info, selftest results and such
    bool is_fan3_used() const;

    /// \returns maximum PWM that is used for cooling in non-emergency situations
    PWM255 max_cooling_pwm() const;

    void set_max_cooling_pwm(PWM255 set);

    /// \returns whether the current configuration allows automatic chamber cooling (cooling fans are not set to a hard value)
    bool can_auto_cool() const;

public: // LEDs
    /// \returns color set for the bed LED strip
    leds::ColorRGBW bed_leds_color() const;

    /// Sets PWM for the led strip that is under the bed
    void set_bed_leds_color(leds::ColorRGBW set);

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

public: // USB
    void set_usb_power(bool enabled);
    bool usb_power() const;

private:
    mutable freertos::Mutex mutex_;

    leds::ColorRGBW bed_leds_color_;

    FanCooling chamber_cooling;

    FanPWMOrAuto cooling_fans_target_pwm_ = pwm_auto;
    FanPWMOrAuto filtration_fan_target_pwm_ = pwm_auto;

    FanPWM cooling_fans_actual_pwm_;
    FanPWM filtration_fan_actual_pwm_;

    // keeps the last timestamp of Fan PWM update
    uint32_t last_fan_update_ms;

    // keeps fan power up timestamp to measure headstart delay
    EnumArray<Fan, uint32_t, xbuddy_extension_shared::fan_count> fan_start_timestamp = {};

    bool can_auto_cool_ = false;
    bool overheating_warning_shown = false;
    bool critical_warning_shown = false;
};

XBuddyExtension &xbuddy_extension();

} // namespace buddy
