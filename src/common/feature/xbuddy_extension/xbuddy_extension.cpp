#include "xbuddy_extension.hpp"

#include <common/temperature.hpp>
#include <common/app_metrics.h>
#include <puppies/xbuddy_extension.hpp>
#include <feature/chamber/chamber.hpp>
#include <feature/xbuddy_extension/cooling.hpp>
#include <leds/side_strip.hpp>
#include <marlin_server.hpp>

namespace buddy {

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
    const auto temp = chamber_temperature();

    // execute control loop only once per defined period
    const auto now_ms = ticks_ms();
    const bool fan_update_pending = (ticks_diff(now_ms, last_fan_update_ms) >= static_cast<int32_t>(chamber_cooling.dt_s * 1000));

    if (fan_update_pending && rpm0.has_value() && rpm1.has_value() && temp.has_value()) {
        last_fan_update_ms = now_ms;
        chamber_cooling.target_temperature = target_temp;
        const bool already_spinning = *rpm0 > 5 && *rpm1 > 5;

        const uint8_t pwm = chamber_cooling.compute_pwm_step(already_spinning, *temp);

        puppies::xbuddy_extension.set_fan_pwm(0, pwm);
        puppies::xbuddy_extension.set_fan_pwm(1, pwm);

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
            const uint16_t pwm = puppies::xbuddy_extension.get_requested_fan_pwm(fan);
            const uint16_t rpm = puppies::xbuddy_extension.get_fan_rpm(fan).value_or(0);
            metric_record_custom(&xbe_fan, ",fan=%d pwm=%ui,rpm=%ui", fan + 1, pwm, rpm);
        }
    }
}

std::optional<uint16_t> XBuddyExtension::fan1_rpm() const {
    return puppies::xbuddy_extension.get_fan_rpm(0);
}

std::optional<uint16_t> XBuddyExtension::fan2_rpm() const {
    return puppies::xbuddy_extension.get_fan_rpm(1);
}

uint8_t XBuddyExtension::fan1_fan2_pwm() const {
    std::lock_guard _lg(mutex_);
    return chamber_cooling.target_pwm;
}

void XBuddyExtension::set_fan1_fan2_pwm(uint8_t pwm) {
    std::lock_guard _lg(mutex_);
    chamber_cooling.set_auto_control(false);
    chamber_cooling.target_pwm = pwm;
}

bool XBuddyExtension::has_fan1_fan2_auto_control() const {
    std::lock_guard _lg(mutex_);
    return chamber_cooling.get_auto_control();
}

void XBuddyExtension::set_fan1_fan2_auto_control() {
    std::lock_guard _lg(mutex_);
    chamber_cooling.set_auto_control(true);
}

std::optional<uint16_t> XBuddyExtension::fan3_rpm() const {
    return puppies::xbuddy_extension.get_fan_rpm(2);
}

uint8_t XBuddyExtension::fan3_pwm() const {
    std::lock_guard _lg(mutex_);
    return fan3_pwm_;
}

void XBuddyExtension::set_fan3_pwm(uint8_t pwm) {
    std::lock_guard _lg(mutex_);
    puppies::xbuddy_extension.set_fan_pwm(2, pwm);
    fan3_pwm_ = pwm;
}

buddy::XBuddyExtension::FanState buddy::XBuddyExtension::get_fan12_state() const {
    std::lock_guard _lg(mutex_);
    auto fanrpms = puppies::xbuddy_extension.get_fans_rpm();
    return FanState {
        .fan1rpm = fanrpms[0],
        .fan2rpm = fanrpms[1],
        .fan12pct = buddy::FanCooling::pwm2pct(chamber_cooling.target_pwm),
        .fan12autocontrol = chamber_cooling.get_auto_control(),
    };
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

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

} // namespace buddy
