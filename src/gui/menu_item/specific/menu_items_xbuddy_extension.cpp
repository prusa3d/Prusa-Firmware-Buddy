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
            : static_cast<int>(FanCooling::pwm2pct(exb.fan1_fan2_pwm())));
}

void MI_XBUDDY_EXTENSION_COOLING_FANS::OnClick() {
    auto &exb = xbuddy_extension();
    if (value() == config().special_value) {
        exb.set_fan1_fan2_auto_control();
    } else {
        exb.set_fan1_fan2_pwm(FanCooling::pct2pwm(value()));
    }
}

// MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX
// =============================================
static constexpr NumericInputConfig chamber_fan_max_percent = {
    .min_value = static_cast<float>(FanCooling::pwm2pct(FanCooling::min_pwm)),
    .max_value = 100.f,
    .special_value = 0.f,
    .unit = Unit::percent,
};

MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX::MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX()
    : WiSpin(FanCooling::pwm2pct(FanCooling::get_soft_max_pwm()),
        chamber_fan_max_percent, _("Chamber Fans Limit")) {
    auto &exb = xbuddy_extension();
    set_is_hidden(exb.status() == XBuddyExtension::Status::disabled);
}

void MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX::OnClick() {
    // need to calculate value in float and round it properly, otherwise set value (in %) and stored value (in PWM duty cycle steps) could differ
    FanCooling::set_soft_max_pwm(static_cast<FanCooling::FanPWM>(FanCooling::pct2pwm(value())));
}

// MI_XBE_FILTRATION_FAN
// =============================================
MI_XBE_FILTRATION_FAN::MI_XBE_FILTRATION_FAN()
    : WiSpin(FanCooling::pwm2pct(xbuddy_extension().fan3_pwm()), numeric_input_config::percent_with_off, _("Filtration Fan")) //
{
}
void MI_XBE_FILTRATION_FAN::OnClick() {
    xbuddy_extension().set_fan3_pwm(FanCooling::pct2pwm(value()));
}

// MI_INFO_XBUDDY_EXTENSION_FAN1
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN1::MI_INFO_XBUDDY_EXTENSION_FAN1()
    : WI_FAN_LABEL_t(_("Chamber Fan 1"),
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
    : WI_FAN_LABEL_t(_("Chamber Fan 2"),
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
    : WI_ICON_SWITCH_OFF_ON_t(xbuddy_extension().usb_power(), _("Camera")) {}

void MI_CAM_USB_PWR::OnChange([[maybe_unused]] size_t old_index) {
    // FIXME: Don't interact with xbuddy_extension directly, but use some common interface, like we have for Chamber API
    xbuddy_extension().set_usb_power(!old_index);
}

void MI_CAM_USB_PWR::Loop() {
    // What does this do?
    set_value(buddy::xbuddy_extension().usb_power(), false);
};
