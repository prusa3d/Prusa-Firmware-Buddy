/**
 * @file screen_menu_diagnostics.cpp
 */

#include "screen_menu_diagnostics.hpp"
#include "DialogMoveZ.hpp"
#include "img_resources.hpp"

ScreenMenuDiagnostics::ScreenMenuDiagnostics()
    : ScreenMenuDiagnostics__(_(label)) {
    header.SetIcon(&img::settings_16x16);
}

void ScreenMenuDiagnostics::ScreenMenuDiagnostics::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
