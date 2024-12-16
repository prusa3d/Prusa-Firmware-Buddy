#include "menu_items_xbuddy_extension.hpp"

#include <feature/xbuddy_extension/xbuddy_extension.hpp>
#include <numeric_input_config_common.hpp>

using namespace buddy;

// MI_XBUDDY_EXTENSION_LIGHTS
// =============================================
MI_XBUDDY_EXTENSION_LIGHTS::MI_XBUDDY_EXTENSION_LIGHTS()
    : WiSpin(static_cast<int>(xbuddy_extension().chamber_leds_pwm()) * 100 / 255,
        numeric_input_config::percent_with_off,
        _("Chamber Lights") //
    ) {

    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

void MI_XBUDDY_EXTENSION_LIGHTS::OnClick() {
    xbuddy_extension().set_chamber_leds_pwm(value() * 255 / 100);
}

// MI_XBUDDY_EXTENSION_CHAMBER_FANS
// =============================================
MI_XBUDDY_EXTENSION_COOLING_FANS::MI_XBUDDY_EXTENSION_COOLING_FANS()
    : WiSpin(0, numeric_input_config::percent_with_auto, _("Chamber Fans")) //
{
    auto &exb = xbuddy_extension();
    set_is_hidden(exb.status() == XBuddyExtension::Status::disabled);

    set_value(
        exb.has_fan1_fan2_auto_control()
            ? *config().special_value
            : static_cast<int>(exb.fan1_fan2_pwm()) * 100 / buddy::FanCooling::max_pwm);
}

void MI_XBUDDY_EXTENSION_COOLING_FANS::OnClick() {
    auto &exb = xbuddy_extension();
    if (value() == config().special_value) {
        exb.set_fan1_fan2_auto_control();
    } else {
        exb.set_fan1_fan2_pwm(value() * buddy::FanCooling::max_pwm / 100);
    }
}

// MI_INFO_XBUDDY_EXTENSION_FAN1
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN1::MI_INFO_XBUDDY_EXTENSION_FAN1()
    : WI_FAN_LABEL_t(_("Cooling Fan 1"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = xbuddy_extension().fan1_fan2_pwm(),
                       .rpm = xbuddy_extension().fan1_rpm(),
                   }; } //
    ) {
    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

// MI_INFO_XBUDDY_EXTENSION_FAN2
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN2::MI_INFO_XBUDDY_EXTENSION_FAN2()
    : WI_FAN_LABEL_t(_("Cooling Fan 2"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = xbuddy_extension().fan1_fan2_pwm(),
                       .rpm = xbuddy_extension().fan2_rpm(),
                   }; } //
    ) {
    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

// MI_INFO_XBUDDY_EXTENSION_FAN3
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN3::MI_INFO_XBUDDY_EXTENSION_FAN3()
    : WI_FAN_LABEL_t(_("Filtration Fan"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = xbuddy_extension().fan3_pwm(),
                       .rpm = xbuddy_extension().fan3_rpm(),
                   }; } //
    ) {
    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

// MI_CAM_USB_PWR
// =============================================
MI_CAM_USB_PWR::MI_CAM_USB_PWR()
    : WI_ICON_SWITCH_OFF_ON_t(buddy::xbuddy_extension().usb_power(), _("Camera")) {}

void MI_CAM_USB_PWR::OnChange([[maybe_unused]] size_t old_index) {
    // FIXME: Don't interact with xbuddy_extension directly, but use some common interface, like we have for Chamber API
    buddy::xbuddy_extension().set_usb_power(!old_index);
}

void MI_CAM_USB_PWR::Loop() {
    // What does this do?
    set_value(buddy::xbuddy_extension().usb_power(), false);
};
