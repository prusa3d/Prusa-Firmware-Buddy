#include "xbuddy_extension.hpp"

#include <utility>

#include <common/temperature.hpp>
#include <common/app_metrics.h>
#include <puppies/xbuddy_extension.hpp>
#include <feature/chamber/chamber.hpp>
#include <feature/chamber_filtration/chamber_filtration.hpp>
#include <feature/xbuddy_extension/cooling.hpp>
#include <leds/side_strip.hpp>
#include <marlin_server.hpp>
#include <buddy/unreachable.hpp>
#include <CFanCtl3Wire.hpp> // for FANCTL_START_TIMEOUT

namespace buddy {

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

XBuddyExtension::XBuddyExtension() {
}

XBuddyExtension::Status XBuddyExtension::status() const {
    return Status::ready;
}

void XBuddyExtension::step() {
    // Obtain these values before locking the mutex.
    // Chamber API is accessing XBuddyExtension in some methods as well, so we might cause a deadlock otherwise.
    // BFW-6274
    const auto target_temp = chamber().target_temperature();
    const auto filtration_backend = chamber_filtration().backend();
    const auto filtration_pwm = chamber_filtration().output_pwm();
    const auto chamber_leds_pwm = leds::side_strip.GetColor(0).w;

    std::lock_guard _lg(mutex_);

    if (status() != Status::ready) {
        return;
    }

    puppies::xbuddy_extension.set_rgbw_led({ bed_leds_color_.r, bed_leds_color_.g, bed_leds_color_.b, bed_leds_color_.w });
    puppies::xbuddy_extension.set_white_led(chamber_leds_pwm);
    puppies::xbuddy_extension.set_usb_power(config_store().xbe_usb_power.get());

    const auto rpm0 = puppies::xbuddy_extension.get_fan_rpm(0);
    const auto rpm1 = puppies::xbuddy_extension.get_fan_rpm(1);
    const auto rpm2 = puppies::xbuddy_extension.get_fan_rpm(2);
    const auto temp = chamber_temperature();

    // Trigger fatal error due to chamber temperature only if we get valid values, that are not reasonable
    if (temp.has_value()) {
        static constexpr Temperature chamber_mintemp = 0.0f;
        static constexpr Temperature chamber_maxtemp = 85.0f;

        if (*temp <= chamber_mintemp) {
            fatal_error(ErrCode::ERR_TEMPERATURE_CHAMBER_MINTEMP);
        } else if (*temp > chamber_maxtemp) {
            fatal_error(ErrCode::ERR_TEMPERATURE_CHAMBER_MAXTEMP);
        }
    }

    // execute control loop only once per defined period
    const auto now_ms = ticks_ms();
    const bool fan_update_pending = (ticks_diff(now_ms, last_fan_update_ms) >= static_cast<int32_t>(chamber_cooling.dt_s * 1000));

    if (fan_update_pending && temp.has_value()) {
        last_fan_update_ms = now_ms;

        const auto max_auto_pwm = max_cooling_pwm();

        switch (filtration_backend) {

        case ChamberFiltrationBackend::xbe_official_filter:
            // The filtration fan does both filtration and cooling
            cooling_fans_actual_pwm_ = cooling_fans_target_pwm_.value_or(FanPWM { 0 });
            filtration_fan_actual_pwm_ = std::max(chamber_cooling.compute_pwm_step(*temp, target_temp, filtration_fan_target_pwm_, max_auto_pwm), filtration_pwm);
            break;

        case ChamberFiltrationBackend::xbe_filter_on_cooling_fans:
            // The cooling fans do both filtration and cooling
            cooling_fans_actual_pwm_ = std::max(chamber_cooling.compute_pwm_step(*temp, target_temp, cooling_fans_target_pwm_, max_auto_pwm), filtration_pwm);
            filtration_fan_actual_pwm_ = filtration_fan_target_pwm_.value_or(FanPWM { 0 });
            break;

        default:
            cooling_fans_actual_pwm_ = chamber_cooling.compute_pwm_step(*temp, target_temp, cooling_fans_target_pwm_, max_auto_pwm);
            filtration_fan_actual_pwm_ = filtration_fan_target_pwm_.value_or(FanPWM { 0 });
            break;
        }

        // Apply emergency & spinup fan control
        cooling_fans_actual_pwm_ = chamber_cooling.apply_pwm_overrides(rpm0.value_or(0) > 5 && rpm1.value_or(0) > 5, cooling_fans_actual_pwm_);
        filtration_fan_actual_pwm_ = chamber_cooling.apply_pwm_overrides(rpm2.value_or(0) > 5, filtration_fan_actual_pwm_);

        puppies::xbuddy_extension.set_fan_pwm(0, cooling_fans_actual_pwm_.value);
        puppies::xbuddy_extension.set_fan_pwm(1, cooling_fans_actual_pwm_.value);
        puppies::xbuddy_extension.set_fan_pwm(2, filtration_fan_actual_pwm_.value);

        if (cooling_fans_actual_pwm_.value == 0) {
            fan_start_timestamp[ftrstd::to_underlying(xbuddy_extension_shared::Fan::cooling_fan_1)] = 0;
            fan_start_timestamp[ftrstd::to_underlying(xbuddy_extension_shared::Fan::cooling_fan_2)] = 0;
        }
        if (filtration_fan_actual_pwm_.value == 0) {
            fan_start_timestamp[ftrstd::to_underlying(xbuddy_extension_shared::Fan::filtration_fan)] = 0;
        }

        if (chamber_cooling.get_critical_temp_flag()) {
            // executed from task marlin_server, marlin_server must be called directly
            if (!critical_warning_shown || marlin_server::is_printing()) {
                marlin_server::print_abort();
                thermalManager.disable_all_heaters();
                marlin_server::set_warning(WarningType::ChamberCriticalTemperature);
                critical_warning_shown = true;
            }
        } else if (chamber_cooling.get_overheating_temp_flag()) {
            // executed from task marlin_server, marlin_server must be called directly
            if (!marlin_server::is_warning_active(WarningType::ChamberOverheatingTemperature) && !overheating_warning_shown) {
                marlin_server::set_warning(WarningType::ChamberOverheatingTemperature);
                overheating_warning_shown = true;
            }
        } else {
            overheating_warning_shown = false;
            critical_warning_shown = false;
        }

    } // else -> comm not working, we'll set it next time (instead of setting
      // them to wrong value, keep them at what they are now).

    METRIC_DEF(xbe_fan, "xbe_fan", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
    static auto fan_should_record = metrics::RunApproxEvery(1000);
    if (fan_should_record()) {
        for (int fan = 0; fan < 3; ++fan) {
            const FanPWM pwm = FanPWM { puppies::xbuddy_extension.get_requested_fan_pwm(fan) };
            const FanRPM rpm = puppies::xbuddy_extension.get_fan_rpm(fan).value_or(0);
            metric_record_custom(&xbe_fan, ",fan=%d pwm=%ui,rpm=%ui", fan + 1, pwm.value, rpm);
        }
    }
}

std::optional<XBuddyExtension::FanRPM> XBuddyExtension::fan_rpm(Fan fan) const {
    return puppies::xbuddy_extension.get_fan_rpm(std::to_underlying(fan));
}

bool XBuddyExtension::is_fan_ok(const Fan fan) {
    const auto actual_pwm = fan_actual_pwm(fan);

    std::lock_guard _lg(mutex_);

    const auto rpm = fan_rpm(fan);
    if (!rpm.has_value() || rpm.value() != 0) {
        return true; // Communication error with puppy || fan IS ok
    }

    auto &start_timestamp = fan_start_timestamp[ftrstd::to_underlying(fan)];
    // fan start delay (to avoid false-positive)
    if (actual_pwm.value > 0 && start_timestamp == 0) {
        start_timestamp = ticks_ms();
    } else {
        // Error state - fan should rotate, but it doesn't
        // Report only if initial start period is over
        if (ticks_diff(ticks_ms(), start_timestamp) >= FANCTL_START_TIMEOUT) {
            return actual_pwm.value == 0;
        }
    }

    return true;
};

XBuddyExtension::FanPWMOrAuto XBuddyExtension::fan_target_pwm(Fan fan) const {
    std::lock_guard _lg(mutex_);

    switch (fan) {
    case Fan::cooling_fan_1:
    case Fan::cooling_fan_2:
        return cooling_fans_target_pwm_;

    case Fan::filtration_fan:
        return filtration_fan_target_pwm_;
    }

    BUDDY_UNREACHABLE();
}

XBuddyExtension::FanPWM XBuddyExtension::fan_actual_pwm(Fan fan) const {
    std::lock_guard _lg(mutex_);

    switch (fan) {
    case Fan::cooling_fan_1:
    case Fan::cooling_fan_2:
        return cooling_fans_actual_pwm_;

    case Fan::filtration_fan:
        return filtration_fan_actual_pwm_;
    }

    BUDDY_UNREACHABLE();
}

void XBuddyExtension::set_fan_target_pwm(Fan fan, FanPWMOrAuto target) {
    std::lock_guard _lg(mutex_);

    switch (fan) {
    case Fan::cooling_fan_1:
    case Fan::cooling_fan_2:
        cooling_fans_target_pwm_ = target;
        return;

    case Fan::filtration_fan:
        filtration_fan_target_pwm_ = target;
        return;
    }

    BUDDY_UNREACHABLE();
}

buddy::XBuddyExtension::FanState buddy::XBuddyExtension::get_fan12_state() const {
    std::lock_guard _lg(mutex_);
    auto fanrpms = puppies::xbuddy_extension.get_fans_rpm();
    return FanState {
        .fan1rpm = fanrpms[0],
        .fan2rpm = fanrpms[1],
        .fan1_fan2_target_pwm = cooling_fans_target_pwm_,
    };
}

PWM255 XBuddyExtension::max_cooling_pwm() const {
    if (chamber_filtration().backend() == ChamberFiltrationBackend::xbe_official_filter) {
        return FanPWM { config_store().xbe_filtration_fan_max_auto_pwm.get() };
    } else {
        return FanPWM { config_store().xbe_cooling_fan_max_auto_pwm.get() };
    }
}

void XBuddyExtension::set_max_cooling_pwm(PWM255 set) {
    if (chamber_filtration().backend() == ChamberFiltrationBackend::xbe_official_filter) {
        config_store().xbe_filtration_fan_max_auto_pwm.set(set.value);
    } else {
        config_store().xbe_cooling_fan_max_auto_pwm.set(set.value);
    }
}

leds::ColorRGBW XBuddyExtension::bed_leds_color() const {
    std::lock_guard _lg(mutex_);
    return bed_leds_color_;
}

void XBuddyExtension::set_bed_leds_color(leds::ColorRGBW set) {
    std::lock_guard _lg(mutex_);
    bed_leds_color_ = set;
}

std::optional<Temperature> XBuddyExtension::chamber_temperature() {
    return puppies::xbuddy_extension.get_chamber_temp();
}

std::optional<XBuddyExtension::FilamentSensorState> XBuddyExtension::filament_sensor() {
    return puppies::xbuddy_extension.get_filament_sensor_state().transform([](auto val) { return static_cast<FilamentSensorState>(val); });
}

void XBuddyExtension::set_usb_power(bool enabled) {
    config_store().xbe_usb_power.set(enabled);
}

bool XBuddyExtension::usb_power() const {
    return config_store().xbe_usb_power.get();
}

} // namespace buddy
