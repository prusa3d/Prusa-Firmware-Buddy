#include "menu_items_xbuddy_extension.hpp"

#include <feature/xbuddy_extension/xbuddy_extension.hpp>
#include <numeric_input_config_common.hpp>

using namespace buddy;

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
