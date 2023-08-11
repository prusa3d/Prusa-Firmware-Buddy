#include "screen_menu_control.hpp"
#include "ScreenFactory.hpp"
#include "img_resources.hpp"
#include "DialogMoveZ.hpp"

ScreenMenuControl::ScreenMenuControl()
    : ScreenMenuControlSpec(_(label)) {
    header.SetIcon(&img::calibrate_white_16x16);
}

void ScreenMenuControl::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuControl() {
    return ScreenFactory::Screen<ScreenMenuControl>();
}
