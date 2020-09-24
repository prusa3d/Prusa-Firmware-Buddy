/*
 * screen_messages.cpp
 *
 *  Created on: Nov 13, 2019
 *      Author: Migi
 */

#include "screen_messages.hpp"
#include "marlin_server.h"
#include "ScreenHandler.hpp"
#include <stdlib.h>
#include <stdint.h>
#include "i18n.h"
#include "gui.hpp"

screen_messages_data_t::screen_messages_data_t()
    : window_frame_t()
    , header(this)
    , footer(this)
    , term(this, GuiDefaults::RectScreenBody.TopLeft(), &term_buff) { // Rect16(10, 28, 11 * 20, 18 * 16))
    header.SetText(_("MESSAGES"));
}

void screen_messages_data_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        Screens::Access()->Close();
    } else {
        window_frame_t::WindowEvent(sender, event, param);
    }

    //must be last window_frame_t could validate term
    while (!MsgCircleBuffer().IsEmpty()) {
        term.Printf("%s\n", MsgCircleBuffer().ConsumeFirst());
    }
}
