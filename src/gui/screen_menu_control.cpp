#include "screen_menu_control.hpp"
#include "ScreenFactory.hpp"
#include "img_resources.hpp"
#include "DialogMoveZ.hpp"
#include <device/board.h>
#if XL_ENCLOSURE_SUPPORT()
    #include "xl_enclosure.hpp"
#endif

ScreenMenuControl::ScreenMenuControl()
    : ScreenMenuControlSpec(_(label)) {
    header.SetIcon(&img::calibrate_white_16x16);
}

void ScreenMenuControl::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }
#if XL_ENCLOSURE_SUPPORT()
    if (event == GUI_event_t::LOOP) {
        if (xl_enclosure.isActive() && Item<MI_ENCLOSURE>().IsHidden()) {
            SwapVisibility<MI_ENCLOSURE, MI_ENCLOSURE_ENABLE>();
        } else if (!xl_enclosure.isActive() && Item<MI_ENCLOSURE_ENABLE>().IsHidden()) {
            SwapVisibility<MI_ENCLOSURE_ENABLE, MI_ENCLOSURE>();
        }
    }
#endif

    ScreenMenu::windowEvent(sender, event, param);
}
