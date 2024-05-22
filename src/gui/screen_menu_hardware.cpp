#include "screen_menu_hardware.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "ScreenHandler.hpp"
#include "sys.h"

ScreenMenuHardware::ScreenMenuHardware()
    : ScreenMenuHardware__(_(label)) {
}

void ScreenMenuHardware::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>);
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

#define NOTRAN(x) string_view_utf8::MakeCPUFLASH((const uint8_t *)x)
