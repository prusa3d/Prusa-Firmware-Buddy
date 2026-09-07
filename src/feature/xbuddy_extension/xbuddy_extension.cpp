#include "xbuddy_extension.hpp"

#include <utility>

#include <common/temperature.hpp>
#include <puppies/xbuddy_extension.hpp>
#include <feature/chamber/chamber.hpp>
#include <feature/chamber_filtration/chamber_filtration.hpp>
#include <marlin_server.hpp>
#include <leds/side_strip_handler.hpp>
#include <bsod/bsod.h>
#include <fanctl/CFanCtl3Wire.hpp> // for FANCTL_START_TIMEOUT
#include <utils/timing/rate_limiter.hpp>

namespace {

// PWM used for the white led in strobe mode. We want more "dark" time than "light" time for clearer stroboscopic effect.
//
// This is a bit below 15%.
constexpr uint8_t strobe_pwm = 35;

// How long a bad RPM reading must persist before is_fan_ok reports failure.
constexpr uint32_t fan_bad_timeout_ms = 750;

} // namespace

namespace buddy {

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

XBuddyExtension::XBuddyExtension() {
#if XBUDDY_EXTENSION_VARIANT_IS_iX()
    buddy::puppies::xbuddy_extension.set_mmu_power(true);
#endif
}

XBuddyExtension::Status XBuddyExtension::status() const {
    return Status::ready;
}

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
void XBuddyExtension::step() {
    // Obtain these values before locking the mutex.
    // Chamber API is accessing XBuddyExtension in some methods as well, so we might cause a deadlock otherwise.
    // BFW-6274
    const auto target_temp = chamber().target_temperature();
    const auto filtration_backend = chamber_filtration().backend();
    const auto filtration_pwm = chamber_filtration().output_pwm();
    const auto temp = chamber().current_temperature();

    std::lock_guard _lg(mutex_);

    const auto chamber_leds_pwm = strobe_freq_.has_value() ? strobe_pwm : leds::SideStripHandler::instance().color().w;

    if (status() != Status::ready) {
        return;
    }

    puppies::xbuddy_extension.set_rgbw_led({ bed_leds_color_.r, bed_leds_color_.g, bed_leds_color_.b, bed_leds_color_.w });
    puppies::xbuddy_extension.set_white_led(chamber_leds_pwm);
    puppies::xbuddy_extension.set_white_strobe_frequency(strobe_freq_);
    puppies::xbuddy_extension.set_usb_power(config_store().xbe_usb_power.get());

    const auto now = ticks_ms();

    const auto update_fan = [this, now](Fan fan, FanPWM pwm) -> std::optional<FanRPM> {
        const auto rpm = puppies::xbuddy_extension.get_fan_rpm(std::to_underlying(fan));

        // No data -> assume the fan is not running; reset the spin-up timer.
        if (!rpm.has_value()) {
            fan_start_timestamp[fan] = now;
        }

        bool sample_bad = false;
        if (pwm.value == 0) {
            // Fan is not supposed to spin
        } else if (!rpm.has_value() || rpm.value() > 0) {
            // Fan is spinning, or RPM unknown (modbus comm error)
        } else if (ticks_diff(now, fan_start_timestamp[fan]) >= FANCTL_START_TIMEOUT) {
            sample_bad = true;
        }
        fan_failure_latch[fan].update(sample_bad, now, fan_bad_timeout_ms);

        return rpm;
    };

    const auto rpm0 = update_fan(Fan::cooling_fan_1, cooling_fans_actual_pwm_);
    const auto rpm1 = update_fan(Fan::cooling_fan_2, cooling_fans_actual_pwm_);
    const auto rpm2 = update_fan(Fan::filtration_fan, filtration_fan_actual_pwm_);

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
            can_auto_cool_ = (filtration_fan_target_pwm_ == pwm_auto);
            break;

        case ChamberFiltrationBackend::xbe_filter_on_cooling_fans:
            // The cooling fans do both filtration and cooling
            cooling_fans_actual_pwm_ = std::max(chamber_cooling.compute_pwm_step(*temp, target_temp, cooling_fans_target_pwm_, max_auto_pwm), filtration_pwm);
            filtration_fan_actual_pwm_ = filtration_fan_target_pwm_.value_or(FanPWM { 0 });
            can_auto_cool_ = (cooling_fans_target_pwm_ == pwm_auto);
            break;

        default:
            cooling_fans_actual_pwm_ = chamber_cooling.compute_pwm_step(*temp, target_temp, cooling_fans_target_pwm_, max_auto_pwm);
            filtration_fan_actual_pwm_ = filtration_fan_target_pwm_.value_or(FanPWM { 0 });
            can_auto_cool_ = (cooling_fans_target_pwm_ == pwm_auto);
            break;
        }

        // Apply emergency & spinup fan control
        cooling_fans_actual_pwm_ = chamber_cooling.apply_pwm_overrides(rpm0.value_or(0) > 5 && rpm1.value_or(0) > 5, cooling_fans_actual_pwm_);
        filtration_fan_actual_pwm_ = chamber_cooling.apply_pwm_overrides(rpm2.value_or(0) > 5, filtration_fan_actual_pwm_);

        const auto set_fan_pwm = [this](Fan fan, PWM255 pwm) {
            const uint8_t fan_idx = std::to_underlying(fan);
            const uint8_t prev_pwm = puppies::xbuddy_extension.get_requested_fan_pwm(fan_idx);
            puppies::xbuddy_extension.set_fan_pwm(fan_idx, pwm.value);

            // Refresh the start timestamp whenever the fan is not running (so
            // it stays fresh while off) and on the 0 -> non-zero edge (so
            // spin-up gets a fresh FANCTL_START_TIMEOUT grace period).
            if (pwm.value == 0 || prev_pwm == 0) {
                fan_start_timestamp[fan] = ticks_ms();
            }
        };

        set_fan_pwm(Fan::cooling_fan_1, cooling_fans_actual_pwm_);
        set_fan_pwm(Fan::cooling_fan_2, cooling_fans_actual_pwm_);
        set_fan_pwm(Fan::filtration_fan, filtration_fan_actual_pwm_);

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
    static auto fan_should_record = RateLimiter<uint32_t>(1000);
    if (fan_should_record.check(ticks_ms())) {
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

bool XBuddyExtension::is_fan_ok(const Fan fan) const {
    std::lock_guard _lg(mutex_);
    return !fan_failure_latch[fan].is_triggered();
}

XBuddyExtension::FanPWMOrAuto XBuddyExtension::fan_target_pwm(Fan fan) const {
    std::lock_guard _lg(mutex_);

    switch (fan) {
    case Fan::cooling_fan_1:
    case Fan::cooling_fan_2:
        return cooling_fans_target_pwm_;

    case Fan::filtration_fan:
        return filtration_fan_target_pwm_;
    }

    bsod_unreachable();
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

    bsod_unreachable();
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

    bsod_unreachable();
}

XBuddyExtension::FanState XBuddyExtension::get_fan12_state() const {
    std::lock_guard _lg(mutex_);
    auto fanrpms = puppies::xbuddy_extension.get_fans_rpm();
    return FanState {
        .fan1rpm = fanrpms[0],
        .fan2rpm = fanrpms[1],
        .fan1_fan2_target_pwm = cooling_fans_target_pwm_,
    };
}

bool XBuddyExtension::using_filtration_fan_instead_of_cooling_fans() const {
    switch (chamber_filtration().backend()) {
    case ChamberFiltrationBackend::xbe_official_filter:
        return true;

    case ChamberFiltrationBackend::none:
    case ChamberFiltrationBackend::xbe_filter_on_cooling_fans:
        return false;
    }

    return false;
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

bool XBuddyExtension::can_auto_cool() const {
    std::lock_guard _lg(mutex_);
    return can_auto_cool_;
}

leds::ColorRGBW XBuddyExtension::bed_leds_color() const {
    std::lock_guard _lg(mutex_);
    return bed_leds_color_;
}

void XBuddyExtension::set_bed_leds_color(leds::ColorRGBW set) {
    std::lock_guard _lg(mutex_);
    bed_leds_color_ = set;
}

void XBuddyExtension::set_strobe(std::optional<uint16_t> freq) {
    debug_assert(freq != 0);

    std::lock_guard _lg(mutex_);
    strobe_freq_ = freq;
}

std::optional<Temperature> XBuddyExtension::chamber_temperature() {
    return puppies::xbuddy_extension.get_chamber_temp();
}

void XBuddyExtension::set_usb_power(bool enabled) {
    config_store().xbe_usb_power.set(enabled);
}

bool XBuddyExtension::usb_power() const {
    return config_store().xbe_usb_power.get();
}

#elif XBUDDY_EXTENSION_VARIANT_IS_iX()
void XBuddyExtension::set_heatbreak_fan_pwm(uint32_t value) {
    if (buddy::puppies::xbuddy_extension.get_requested_fan_pwm(0) == 0 && value > 0) {
        std::lock_guard guard(mutex_);
        hbr_fan_start_timestamp = ticks_ms();
    }

    // Fan 1 and Fan 2 on xbe share PWM, but the interface is schizophrenic, we need to set both
    buddy::puppies::xbuddy_extension.set_fan_pwm(0, value);
    buddy::puppies::xbuddy_extension.set_fan_pwm(1, value);
}

uint32_t XBuddyExtension::get_heatbreak_fan_pwm() {
    return buddy::puppies::xbuddy_extension.get_requested_fan_pwm(0);
}

uint32_t XBuddyExtension::get_heatbreak_fan_rpm() {
    return buddy::puppies::xbuddy_extension.get_fan_rpm(0).value_or(0);
}

bool XBuddyExtension::is_heatbreak_fan_ok() {
    const auto pwm = buddy::puppies::xbuddy_extension.get_requested_fan_pwm(0);
    const auto rpm = buddy::puppies::xbuddy_extension.get_fan_rpm(0);

    std::lock_guard guard(mutex_);

    if (pwm == 0) {
        // Fan is not supposed to spin - all is well
    } else if (rpm && *rpm > 0) {
        // Fan is spinning - all is well
    } else if (ticks_diff(ticks_ms(), hbr_fan_start_timestamp) >= FANCTL_START_TIMEOUT) {
        // Fan should be spinning for some time now, but it isn't -> report a problem
        return false;
    }

    return true;
}

void XBuddyExtension::set_white_led(uint32_t intensity) {
    std::lock_guard guard(mutex_);

    if (white_intensity_override) {
        intensity = *white_intensity_override;
    }
    buddy::puppies::xbuddy_extension.set_white_led(intensity);
}

void XBuddyExtension::set_strobe(std::optional<uint16_t> freq) {
    debug_assert(freq != 0);
    std::lock_guard guard(mutex_);

    if (freq) {
        white_intensity_override = strobe_pwm;
    } else {
        white_intensity_override = std::nullopt;
    }
    buddy::puppies::xbuddy_extension.set_white_led(*white_intensity_override);
    buddy::puppies::xbuddy_extension.set_white_strobe_frequency(freq);
}

void XBuddyExtension::set_rgbw_led(leds::ColorRGBW rgbw) {
    buddy::puppies::xbuddy_extension.set_rgbw_led({ rgbw.r, rgbw.g, rgbw.b, rgbw.w });
}
#endif

std::optional<XBuddyExtension::FilamentSensorState> XBuddyExtension::gpio_filament_sensor() const {
    return puppies::xbuddy_extension.get_gpio_filament_sensor_state();
}

std::optional<XBuddyExtension::FilamentSensorState> XBuddyExtension::ext_filament_sensor(uint8_t index) const {
    return puppies::xbuddy_extension.get_ext_filament_sensor_state(index);
}

} // namespace buddy
