#include "menu_items_xbuddy_extension.hpp"

#include <feature/xbuddy_extension/xbuddy_extension.hpp>
#include <numeric_input_config_common.hpp>
#include <feature/xbuddy_extension/cooling.hpp>

using namespace buddy;

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
            : exb.fan1_fan2_pwm().to_percent());
}

void MI_XBUDDY_EXTENSION_COOLING_FANS::OnClick() {
    auto &exb = xbuddy_extension();
    if (value() == config().special_value) {
        exb.set_fan1_fan2_auto_control();
    } else {
        exb.set_fan1_fan2_pwm(XBuddyExtension::FanPWM::from_percent(value()));
    }
}

// MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX
// =============================================
static constexpr NumericInputConfig chamber_fan_max_percent = {
    .min_value = FanCooling::min_pwm.to_percent(),
    .max_value = 100.f,
    .special_value = 0.f,
    .unit = Unit::percent,
};

MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX::MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX()
    : WiSpin(FanCooling::get_soft_max_pwm().to_percent(), chamber_fan_max_percent, _("Chamber Fans Limit")) {
    auto &exb = xbuddy_extension();
    set_is_hidden(exb.status() == XBuddyExtension::Status::disabled);
}

void MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX::OnClick() {
    // need to calculate value in float and round it properly, otherwise set value (in %) and stored value (in PWM duty cycle steps) could differ
    FanCooling::set_soft_max_pwm(XBuddyExtension::FanPWM::from_percent(value()));
}

// MI_XBE_FILTRATION_FAN
// =============================================
MI_XBE_FILTRATION_FAN::MI_XBE_FILTRATION_FAN()
    : WiSpin(xbuddy_extension().fan3_pwm().to_percent(), numeric_input_config::percent_with_off, _("Filtration Fan")) //
{
}
void MI_XBE_FILTRATION_FAN::OnClick() {
    xbuddy_extension().set_fan3_pwm(XBuddyExtension::FanPWM::from_percent(value()));
}

// MI_INFO_XBUDDY_EXTENSION_FAN1
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN1::MI_INFO_XBUDDY_EXTENSION_FAN1()
    : WI_FAN_LABEL_t(_("Chamber Fan 1"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = xbuddy_extension().fan1_fan2_pwm().value,
                       .rpm = xbuddy_extension().fan1_rpm(),
                   }; } //
    ) {
    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

// MI_INFO_XBUDDY_EXTENSION_FAN2
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN2::MI_INFO_XBUDDY_EXTENSION_FAN2()
    : WI_FAN_LABEL_t(_("Chamber Fan 2"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = xbuddy_extension().fan1_fan2_pwm().value,
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
                       .pwm = xbuddy_extension().fan3_pwm().value,
                       .rpm = xbuddy_extension().fan3_rpm(),
                   }; } //
    ) {
    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

// MI_CAM_USB_PWR
// =============================================
MI_CAM_USB_PWR::MI_CAM_USB_PWR()
    : WI_ICON_SWITCH_OFF_ON_t(xbuddy_extension().usb_power(), _("Camera")) {}

void MI_CAM_USB_PWR::OnChange([[maybe_unused]] size_t old_index) {
    // FIXME: Don't interact with xbuddy_extension directly, but use some common interface, like we have for Chamber API
    xbuddy_extension().set_usb_power(!old_index);
}

void MI_CAM_USB_PWR::Loop() {
    // What does this do?
    set_value(buddy::xbuddy_extension().usb_power(), false);
};
