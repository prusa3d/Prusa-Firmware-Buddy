#include "menu_items_xbuddy_extension.hpp"

#include <feature/xbuddy_extension/xbuddy_extension.hpp>
#include <numeric_input_config_common.hpp>
#include <feature/chamber/chamber.hpp>

using namespace buddy;

namespace {
void setup_fan_item(WiSpin &item, XBuddyExtension::Fan fan) {
    auto &exb = xbuddy_extension();
    item.set_is_hidden(exb.status() == XBuddyExtension::Status::disabled);

    const auto tgt = exb.fan_target_pwm(fan);
    item.set_value(tgt.transform(XBuddyExtension::FanPWM::to_percent_static).value_or(*item.config().special_value));
}

void handle_fan_item_click(WiSpin &item, XBuddyExtension::Fan fan) {
    XBuddyExtension::FanPWMOrAuto tgt = pwm_auto;
    if (const auto val = item.value_opt()) {
        tgt = XBuddyExtension::FanPWM::from_percent(*val);
    }
    xbuddy_extension().set_fan_target_pwm(fan, tgt);
}

template <XBuddyExtension::Fan fan>
FanPWMAndRPM fan_info_function(auto) {
    return FanPWMAndRPM {
        .pwm = xbuddy_extension().fan_actual_pwm(fan).value,
        .rpm = xbuddy_extension().fan_rpm(fan),
    };
}
} // namespace

// MI_XBUDDY_EXTENSION_CHAMBER_FANS
// =============================================
MI_XBUDDY_EXTENSION_COOLING_FANS::MI_XBUDDY_EXTENSION_COOLING_FANS()
    : WiSpin(0, numeric_input_config::percent_with_auto, _("Chamber Fans")) //
{
    setup_fan_item(*this, XBuddyExtension::Fan::cooling_fan_1);
}

void MI_XBUDDY_EXTENSION_COOLING_FANS::OnClick() {
    handle_fan_item_click(*this, XBuddyExtension::Fan::cooling_fan_1);
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
    : WiSpin(0, numeric_input_config::percent_with_auto, _("Filtration Fan")) //
{
    setup_fan_item(*this, XBuddyExtension::Fan::filtration_fan);
}

void MI_XBE_FILTRATION_FAN::OnClick() {
    handle_fan_item_click(*this, XBuddyExtension::Fan::filtration_fan);
}

// MI_INFO_XBUDDY_EXTENSION_FAN1
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN1::MI_INFO_XBUDDY_EXTENSION_FAN1()
    : WI_FAN_LABEL_t(_("Chamber Fan 1"), fan_info_function<XBuddyExtension::Fan::cooling_fan_1>) {
    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

// MI_INFO_XBUDDY_EXTENSION_FAN2
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN2::MI_INFO_XBUDDY_EXTENSION_FAN2()
    : WI_FAN_LABEL_t(_("Chamber Fan 2"), fan_info_function<XBuddyExtension::Fan::cooling_fan_2>) {
    set_is_hidden(xbuddy_extension().status() == XBuddyExtension::Status::disabled);
}

// MI_INFO_XBUDDY_EXTENSION_FAN3
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN3::MI_INFO_XBUDDY_EXTENSION_FAN3()
    : WI_FAN_LABEL_t(_("Filtration fan"), fan_info_function<XBuddyExtension::Fan::filtration_fan>) {
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
