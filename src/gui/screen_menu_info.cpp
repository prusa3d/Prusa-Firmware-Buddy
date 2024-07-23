/**
 * @file screen_menu_info.cpp
 */

#include "screen_menu_info.hpp"
#include "img_resources.hpp"
#include "DialogMoveZ.hpp"

void ScreenMenuInfo::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

ScreenMenuInfo::ScreenMenuInfo()
    : ScreenMenuInfo__(_(label)) {
    EnableLongHoldScreenAction();
#if (!PRINTER_IS_PRUSA_MINI())
    header.SetIcon(&img::info_16x16);
#endif // PRINTER_IS_PRUSA_MINI()
}
