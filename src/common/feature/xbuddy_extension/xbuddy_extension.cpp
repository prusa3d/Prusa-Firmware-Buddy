#include "xbuddy_extension.hpp"

#include <common/temperature.hpp>
#include <common/app_metrics.h>
#include <puppies/xbuddy_extension.hpp>
#include <feature/chamber/chamber.hpp>

namespace buddy {

XBuddyExtension::XBuddyExtension() {
}

XBuddyExtension::Status XBuddyExtension::status() const {
    return Status::ready;
}

void XBuddyExtension::step() {
    std::lock_guard _lg(mutex_);

    if (status() != Status::ready) {
        return;
    }

    update_registers_nolock();
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
    chamber_cooling.auto_control = false;
    chamber_cooling.target_pwm = pwm;
    update_registers_nolock();
}

bool XBuddyExtension::has_fan1_fan2_auto_control() const {
    std::lock_guard _lg(mutex_);
    return chamber_cooling.auto_control;
}

void XBuddyExtension::set_fan1_fan2_auto_control() {
    std::lock_guard _lg(mutex_);
    chamber_cooling.auto_control = true;
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

leds::ColorRGBW XBuddyExtension::bed_leds_color() const {
    std::lock_guard _lg(mutex_);
    return bed_leds_color_;
}

void XBuddyExtension::set_bed_leds_color(leds::ColorRGBW set) {
    std::lock_guard _lg(mutex_);
    bed_leds_color_ = set;
    update_registers_nolock();
}

uint8_t XBuddyExtension::chamber_leds_pwm() {
    return config_store().xbuddy_extension_chamber_leds_pwm.get();
}

void XBuddyExtension::set_chamber_leds_pwm(uint8_t set) {
    std::lock_guard _lg(mutex_);

    config_store().xbuddy_extension_chamber_leds_pwm.set(set);
    update_registers_nolock();
}

std::optional<float> XBuddyExtension::chamber_temperature() {
    return puppies::xbuddy_extension.get_chamber_temp();
}

void XBuddyExtension::update_registers_nolock() {
    puppies::xbuddy_extension.set_rgbw_led({ bed_leds_color_.r, bed_leds_color_.g, bed_leds_color_.b, bed_leds_color_.w });
    puppies::xbuddy_extension.set_white_led(config_store().xbuddy_extension_chamber_leds_pwm.get());

    const auto rpm0 = puppies::xbuddy_extension.get_fan_rpm(0);
    const auto rpm1 = puppies::xbuddy_extension.get_fan_rpm(1);
    const auto rpm2 = puppies::xbuddy_extension.get_fan_rpm(2);
    const auto temp = chamber_temperature();

    if (rpm0.has_value() && rpm1.has_value() && temp.has_value()) {
        chamber_cooling.target_temperature = chamber().target_temperature();

        const bool already_spinning = *rpm0 > 5 && *rpm1 > 5;

        const uint8_t pwm = chamber_cooling.compute_pwm(already_spinning, *temp);

        puppies::xbuddy_extension.set_fan_pwm(0, pwm);
        puppies::xbuddy_extension.set_fan_pwm(1, pwm);

        METRIC_DEF(chamber_fan_pwm, "chamber_fan_pwm", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
        static auto pwm_should_record = metrics::RunApproxEvery(1000);
        if (pwm_should_record()) {
            metric_record_custom(&chamber_fan_pwm, "a=%" PRIu8 ",b=%" PRIu8 ",c=%" PRIu8, pwm, pwm, fan3_pwm_);
        }
    } // else -> comm not working, we'll set it next time (instead of setting
      // them to wrong value, keep them at what they are now).

    METRIC_DEF(chamber_fan_rpm, "chamber_fan_rpm", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
    static auto rpm_should_record = metrics::RunApproxEvery(1000);
    if (rpm0.has_value() && rpm1.has_value() && rpm2.has_value() && rpm_should_record()) {
        metric_record_custom(&chamber_fan_rpm, "a=%" PRIu16 ",b=%" PRIu16 ",c=%" PRIu16, *rpm0, *rpm1, *rpm2);
    }
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

} // namespace buddy
