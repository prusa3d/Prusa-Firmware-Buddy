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
    : WiSpin(0, numeric_input_config::percent_with_auto, _("Cooling Fans")) //
{
    auto &exb = xbuddy_extension();
    set_is_hidden(exb.status() == XBuddyExtension::Status::disabled);

    set_value(
        exb.has_fan1_fan2_auto_control()
            ? *config().special_value
            : static_cast<int>(exb.fan1_fan2_pwm()) * 100 / 255);
}

void MI_XBUDDY_EXTENSION_COOLING_FANS::OnClick() {
    auto &exb = xbuddy_extension();
    if (value() == config().special_value) {
        exb.set_fan1_fan2_auto_control();
    } else {
        exb.set_fan1_fan2_pwm(value() * 255 / 100);
    }
}
