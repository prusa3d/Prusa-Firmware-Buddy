#include "xbuddy_extension.hpp"

#include <common/temperature.hpp>
#include <puppies/xbuddy_extension.hpp>
#include <feature/chamber/chamber.hpp>

namespace buddy {

XBuddyExtension::XBuddyExtension() {
    update_registers_nolock();
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
    return cooling.require_pwm;
}

void XBuddyExtension::set_fan1_fan2_pwm(uint8_t pwm) {
    std::lock_guard _lg(mutex_);
    cooling.auto_control = false;
    cooling.require_pwm = pwm;
    update_registers_nolock();
}

bool XBuddyExtension::has_fan1_fan2_auto_control() const {
    std::lock_guard _lg(mutex_);
    return cooling.auto_control;
}

void XBuddyExtension::set_fan1_fan2_auto_control() {
    std::lock_guard _lg(mutex_);
    cooling.auto_control = true;
}

std::optional<uint16_t> XBuddyExtension::fan3_rpm() const {
    return puppies::xbuddy_extension.get_fan_rpm(2);
}

void XBuddyExtension::set_fan3_pwm(uint8_t pwm) {
    puppies::xbuddy_extension.set_fan_pwm(2, pwm);
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
    const auto temp = chamber_temperature();

    if (rpm0.has_value() && rpm1.has_value() && temp.has_value()) {
        cooling.target = chamber().target_temperature();

        const bool already_spinning = *rpm0 > 5 && *rpm1 > 5;

        const uint8_t pwm = cooling.pwm(already_spinning, *temp);

        puppies::xbuddy_extension.set_fan_pwm(0, pwm);
        puppies::xbuddy_extension.set_fan_pwm(1, pwm);
    } // else -> comm not working, we'll set it next time (instead of setting
      // them to wrong value, keep them at what they are now).
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

} // namespace buddy
