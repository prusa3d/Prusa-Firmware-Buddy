#include "xbuddy_extension.hpp"

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

    if (fan1_fan2_auto_control_) {
        // Dummy, untested implementation.
        const auto temp = chamber_temperature();
        const auto target_temp = chamber().target_temperature();

        /// Target-current temperature difference at which the fans go on full
        static constexpr int fans_max_temp_diff = 10;
        fan1_fan2_pwm_ = //
            (!temp.has_value() || !target_temp.has_value())

            // We don't know a temperature or don't have a target set -> do not cool
            ? 0

            // Linearly increase fans up to the fans_max_temp_diff temperature difference
            : std::clamp<int>(static_cast<int>(*temp - *target_temp) * 255 / fans_max_temp_diff, 0, 255);

        update_registers_nolock();
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
    return fan1_fan2_pwm_;
}

void XBuddyExtension::set_fan1_fan2_pwm(uint8_t pwm) {
    std::lock_guard _lg(mutex_);
    fan1_fan2_auto_control_ = false;
    fan1_fan2_pwm_ = pwm;
    update_registers_nolock();
}

bool XBuddyExtension::has_fan1_fan2_auto_control() const {
    std::lock_guard _lg(mutex_);
    return fan1_fan2_auto_control_;
}

void XBuddyExtension::set_fan1_fan2_auto_control() {
    std::lock_guard _lg(mutex_);
    fan1_fan2_auto_control_ = true;
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

    puppies::xbuddy_extension.set_fan_pwm(0, fan1_fan2_pwm_);
    puppies::xbuddy_extension.set_fan_pwm(1, fan1_fan2_pwm_);
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

} // namespace buddy
