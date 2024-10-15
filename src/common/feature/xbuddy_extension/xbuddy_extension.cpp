#include "xbuddy_extension.hpp"

// Stubbed for now, will be properly implemented later

static uint8_t fan1_2_pwm = 0;
static uint8_t fan3_pwm = 0;
static leds::ColorRGBW bed_leds_color;

namespace buddy {

XBuddyExtension::Status XBuddyExtension::status() const {
    return Status::ready;
}

std::optional<uint16_t> XBuddyExtension::fan1_rpm() const {
    return fan1_2_pwm > 0 ? 500 : 0;
}

std::optional<uint16_t> XBuddyExtension::fan2_rpm() const {
    return fan1_2_pwm > 0 ? 500 : 0;
}

void XBuddyExtension::set_fan1_fan2_pwm(uint8_t pwm) {
    fan1_2_pwm = pwm;
}

std::optional<uint16_t> XBuddyExtension::fan3_rpm() const {
    return fan3_pwm > 0 ? 500 : 0;
}

void XBuddyExtension::set_fan3_pwm(uint8_t pwm) {
    fan3_pwm = pwm;
}

leds::ColorRGBW XBuddyExtension::bed_leds_color() const {
    return ::bed_leds_color;
}

void XBuddyExtension::set_bed_leds_color(leds::ColorRGBW set) {
    ::bed_leds_color = set;
}

uint8_t XBuddyExtension::chamber_leds_pwm() {
    return config_store().xbuddy_extension_chamber_leds_pwm.get();
}

void XBuddyExtension::set_chamber_leds_pwm(uint8_t set) {
    config_store().xbuddy_extension_chamber_leds_pwm.set(set);
}

std::optional<uint8_t> XBuddyExtension::chamber_temperature() {
    return 50;
}

void XBuddyExtension::update_registers() {
    // TODO: Update PWM and such
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension instance;
    return instance;
}

} // namespace buddy
