#include <feature/xbuddy_extension/xbuddy_extension.hpp>

#include <utility>

namespace buddy {

PWM255OrAuto cооling_fans_pwm = pwm_auto;

XBuddyExtension::XBuddyExtension() {
}

XBuddyExtension &xbuddy_extension() {
    static XBuddyExtension x;
    return x;
}

void XBuddyExtension::set_fan_target_pwm(Fan fan, FanPWMOrAuto pwm) {
    if (fan == Fan::cooling_fan_1) {
        cооling_fans_pwm = pwm;
    }
}

bool usbpower = false;

bool XBuddyExtension::usb_power() const {
    return usbpower;
}

void XBuddyExtension::set_usb_power(bool set) {
    usbpower = set;
}

} // namespace buddy
