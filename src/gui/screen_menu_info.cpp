/**
 * @file screen_menu_info.cpp
 */

#include "screen_menu_info.hpp"
#include "png_resources.hpp"
#include "DialogMoveZ.hpp"

void ScreenMenuInfo::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenMenuInfo::ScreenMenuInfo()
    : ScreenMenuInfo__(_(label)) {
    EnableLongHoldScreenAction();
#if (PRINTER_TYPE != PRINTER_PRUSA_MINI)
    header.SetIcon(&png::info_16x16);
#endif //PRINTER_PRUSA_MINI
}
