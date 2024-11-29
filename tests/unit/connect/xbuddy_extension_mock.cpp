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

} // namespace buddy
