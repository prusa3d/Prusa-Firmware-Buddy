/*
 * screen_sysinf.cpp
 *
 *  Created on: 2019-09-25
 *      Author: Radek Vana
 */

#include "screen_snake.hpp"
#include "display.h"
#include "ScreenHandler.hpp"

/// this defines movement speed in miliseconds
const constexpr movement_delay = 300;

screen_snake_data_t::screen_snake_data_t()
    : AddSuperWindow<screen_t>() {
}

void unconditionalDraw() {
    display::FillRect(Rect16(10, 10, 100, 100), COLOR_BLUE);
}

void screen_snake_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        Screens::Access()->Close();
    }

    uint32_t now = HAL_GetTick();
    if (now - last_redraw > movement_delay) {
        last_redraw = now;
        /// shift snake and redraw
    }

    SuperWindowEvent(sender, event, param);
}
