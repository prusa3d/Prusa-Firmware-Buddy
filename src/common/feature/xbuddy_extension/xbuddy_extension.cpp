#include "xbuddy_extension.hpp"

#include <puppies/xbuddy_extension.hpp>
#include <feature/chamber/chamber.hpp>

static leds::ColorRGBW bed_leds_color;

namespace buddy {

XBuddyExtension::Status XBuddyExtension::status() const {
    return Status::ready;
}

void XBuddyExtension::step() {
    if (status() != Status::ready) {
        return;
    }

    // Dummy, untested implementation.
    const auto temp = chamber_temperature();
    const auto target_temp = chamber().target_temperature();

    /// Target-current temperature difference at which the fans go on full
    static constexpr int fans_max_temp_diff = 10;
    const auto target_fan_pwm = //
        (!temp.has_value() || !target_temp.has_value())

        // We don't know a temperature or don't have a target set -> do not cool
        ? 0

        // Linearly increase fans up to the fans_max_temp_diff temperature difference
        : std::clamp<int>(static_cast<int>(*temp - *target_temp) * 255 / fans_max_temp_diff, 0, 255);

    set_fan1_fan2_pwm(target_fan_pwm);
}

std::optional<uint16_t> XBuddyExtension::fan1_rpm() const {
    return puppies::xbuddy_extension.get_fan_rpm(0);
}

std::optional<uint16_t> XBuddyExtension::fan2_rpm() const {
    return puppies::xbuddy_extension.get_fan_rpm(1);
}

void XBuddyExtension::set_fan1_fan2_pwm(uint8_t pwm) {
    puppies::xbuddy_extension.set_fan_pwm(0, pwm);
    puppies::xbuddy_extension.set_fan_pwm(1, pwm);
}

std::optional<uint16_t> XBuddyExtension::fan3_rpm() const {
    return puppies::xbuddy_extension.get_fan_rpm(2);
}

void XBuddyExtension::set_fan3_pwm(uint8_t pwm) {
    puppies::xbuddy_extension.set_fan_pwm(2, pwm);
}

leds::ColorRGBW XBuddyExtension::bed_leds_color() const {
    return ::bed_leds_color;
}

void XBuddyExtension::set_bed_leds_color(leds::ColorRGBW set) {
    ::bed_leds_color = set;
    puppies::xbuddy_extension.set_rgbw_led({ set.r, set.g, set.b, set.w });
}

uint8_t XBuddyExtension::chamber_leds_pwm() {
    return config_store().xbuddy_extension_chamber_leds_pwm.get();
}

void XBuddyExtension::set_chamber_leds_pwm(uint8_t set) {
    config_store().xbuddy_extension_chamber_leds_pwm.set(set);
    puppies::xbuddy_extension.set_white_led(set);
}

std::optional<float> XBuddyExtension::chamber_temperature() {
    return puppies::xbuddy_extension.get_chamber_temp();
}

void XBuddyExtension::update_registers() {
    // TODO: Update PWM and such
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

} // namespace buddy
