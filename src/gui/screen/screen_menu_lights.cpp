/// @file
#include "screen_menu_lights.hpp"

#include <img_resources.hpp>

ScreenMenuLights::ScreenMenuLights()
    : ScreenMenuLightsBase {
        _("LIGHTS"),
        &img::settings_16x16,
    } {
    EnableLongHoldScreenAction();
}
