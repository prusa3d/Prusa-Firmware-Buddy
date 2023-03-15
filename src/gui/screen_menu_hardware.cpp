/**
 * @file screen_menu_hardware.cpp
 */

#include "screen_menu_hardware.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "ScreenHandler.hpp"

ScreenMenuHardware::ScreenMenuHardware()
    : ScreenMenuHardware__(_(label)) {
}

void ScreenMenuHardware::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>);
        return;
    }

    SuperWindowEvent(sender, event, param);
}
