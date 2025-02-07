#include <feature/xbuddy_extension/xbuddy_extension.hpp>

namespace buddy {

std::optional<XBuddyExtension::FanPWM> fan12pwm = std::nullopt;

XBuddyExtension::XBuddyExtension() {
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension x;
    return x;
}

void XBuddyExtension::set_fan1_fan2_pwm(FanPWM pwm) {
    fan12pwm = pwm;
}

void XBuddyExtension::set_fan1_fan2_auto_control() {
    fan12pwm.reset();
}

bool usbpower = false;

bool XBuddyExtension::usb_power() const {
    return usbpower;
}

void XBuddyExtension::set_usb_power(bool set) {
    usbpower = set;
}

} // namespace buddy
