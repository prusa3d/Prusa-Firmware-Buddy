#include "menu_items_xbuddy_extension.hpp"

#include <feature/xbuddy_extension/xbuddy_extension.hpp>
#include <numeric_input_config_common.hpp>
#include <feature/chamber/chamber.hpp>
#include <feature/chamber_filtration/chamber_filtration.hpp>

namespace {
void setup_fan_item(WiSpin &item, buddy::XBuddyExtension::Fan fan) {
    auto &exb = buddy::xbuddy_extension();
    item.set_is_hidden(exb.status() == buddy::XBuddyExtension::Status::disabled);

    const auto tgt = exb.fan_target_pwm(fan);
    item.set_value(tgt.transform(buddy::XBuddyExtension::FanPWM::to_percent_static).value_or(*item.config().special_value));
}

void handle_fan_item_click(WiSpin &item, buddy::XBuddyExtension::Fan fan) {
    buddy::XBuddyExtension::FanPWMOrAuto tgt = pwm_auto;
    if (const auto val = item.value_opt()) {
        tgt = buddy::XBuddyExtension::FanPWM::from_percent(static_cast<int8_t>(*val));
    }
    buddy::xbuddy_extension().set_fan_target_pwm(fan, tgt);
}

template <buddy::XBuddyExtension::Fan fan>
FanPWMAndRPM fan_info_function(auto) {
    return FanPWMAndRPM {
        .pwm = buddy::xbuddy_extension().fan_actual_pwm(fan).value,
        .rpm = buddy::xbuddy_extension().fan_rpm(fan),
    };
}

bool disabled_cooling_fans() {
    return buddy::xbuddy_extension().using_filtration_fan_instead_of_cooling_fans();
}
} // namespace

// MI_XBUDDY_EXTENSION_COOLING_FANS
// =============================================
MI_XBUDDY_EXTENSION_COOLING_FANS::MI_XBUDDY_EXTENSION_COOLING_FANS()
    : WiSpin(0, disabled_cooling_fans() ? numeric_input_config::percent_with_disabled : numeric_input_config::percent_with_auto, _("Chamber Fans")) //
{
    setup_fan_item(*this, buddy::XBuddyExtension::Fan::cooling_fan_1);
}

void MI_XBUDDY_EXTENSION_COOLING_FANS::OnClick() {
    handle_fan_item_click(*this, buddy::XBuddyExtension::Fan::cooling_fan_1);
}

// MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX
// =============================================
static constexpr NumericInputConfig chamber_fan_max_percent = {
    .min_value = buddy::FanCooling::min_pwm.to_percent(),
    .max_value = 100.f,
    .special_value = 0.f,
    .unit = Unit::percent,
};

MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX::MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX()
    : WiSpin(buddy::xbuddy_extension().max_cooling_pwm().to_percent(), chamber_fan_max_percent, _("Chamber Fans Limit")) {
    auto &exb = buddy::xbuddy_extension();
    set_is_hidden(exb.status() == buddy::XBuddyExtension::Status::disabled);
}

void MI_XBUDDY_EXTENSION_COOLING_FANS_CONTROL_MAX::OnClick() {
    // need to calculate value in float and round it properly, otherwise set value (in %) and stored value (in PWM duty cycle steps) could differ
    buddy::xbuddy_extension().set_max_cooling_pwm(buddy::XBuddyExtension::FanPWM::from_percent(static_cast<int8_t>(value())));
}

// MI_XBE_FILTRATION_FAN
// =============================================
MI_XBE_FILTRATION_FAN::MI_XBE_FILTRATION_FAN()
    : WiSpin(0, numeric_input_config::percent_with_auto, _("Filtration Fan")) //
{
    setup_fan_item(*this, buddy::XBuddyExtension::Fan::filtration_fan);
}

void MI_XBE_FILTRATION_FAN::OnClick() {
    handle_fan_item_click(*this, buddy::XBuddyExtension::Fan::filtration_fan);
}

// translation: menu item showing info about chamber cooling fan, %d is the fan number
static constexpr const char *chamber_fan_label_template = N_("Chamber Fan %d");

// MI_INFO_XBUDDY_EXTENSION_FAN1
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN1::MI_INFO_XBUDDY_EXTENSION_FAN1()
    : WI_FAN_LABEL_t(string_view_utf8 {}, fan_info_function<buddy::XBuddyExtension::Fan::cooling_fan_1>) {
    SetLabel(_(chamber_fan_label_template).formatted(label_params_, 1));
    set_is_hidden(buddy::xbuddy_extension().status() == buddy::XBuddyExtension::Status::disabled);
}

// MI_INFO_XBUDDY_EXTENSION_FAN2
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN2::MI_INFO_XBUDDY_EXTENSION_FAN2()
    : WI_FAN_LABEL_t(string_view_utf8 {}, fan_info_function<buddy::XBuddyExtension::Fan::cooling_fan_2>) {
    SetLabel(_(chamber_fan_label_template).formatted(label_params_, 2));
    set_is_hidden(buddy::xbuddy_extension().status() == buddy::XBuddyExtension::Status::disabled);
}

// MI_INFO_XBUDDY_EXTENSION_FAN3
// =============================================
MI_INFO_XBUDDY_EXTENSION_FAN3::MI_INFO_XBUDDY_EXTENSION_FAN3()
    : WI_FAN_LABEL_t(_("Filtration Fan"), fan_info_function<buddy::XBuddyExtension::Fan::filtration_fan>) {
    set_is_hidden(buddy::xbuddy_extension().status() == buddy::XBuddyExtension::Status::disabled);
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
    set_value(buddy::xbuddy_extension().usb_power());
};
