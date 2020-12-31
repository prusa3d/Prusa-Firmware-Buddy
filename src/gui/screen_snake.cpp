/*
 * screen_sysinf.cpp
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "screen_snake.hpp"
#include "display.h"

screen_snake_data_t::screen_snake_data_t()
    : AddSuperWindow<screen_t>() {
    display::FillRect(Rect16(10, 10, 100, 100), COLOR_BLUE);
}

void screen_snake_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        Screens::Access()->Close();
    }
    SuperWindowEvent(sender, event, param);
}
