#include <feature/xbuddy_extension/xbuddy_extension.hpp>

namespace buddy {

std::optional<uint8_t> fan12pwm = std::nullopt;

XBuddyExtension::XBuddyExtension() {
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension x;
    return x;
}

void XBuddyExtension::set_fan1_fan2_pwm(uint8_t pwm) {
    fan12pwm = pwm;
}

void XBuddyExtension::set_fan1_fan2_auto_control() {
    fan12pwm.reset();
}

uint8_t ledpwm = 0;

uint8_t XBuddyExtension::chamber_leds_pwm() {
    return ledpwm;
}

void XBuddyExtension::set_chamber_leds_pwm(uint8_t set) {
    ledpwm = set;
}

} // namespace buddy
