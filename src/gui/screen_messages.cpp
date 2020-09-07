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

void screen_messages_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {

    //if (!term.IsInvalid() && !MsgCircleBuffer().IsEmpty()) {
    while (!MsgCircleBuffer().IsEmpty()) {
        term.Printf("%s\n", MsgCircleBuffer().ConsumeFirst());
        term.Invalidate();
    }

    if (event == WINDOW_EVENT_CLICK) {
        Screens::Access()->Close();
    } else {
        window_frame_t::windowEvent(sender, event, param);
    }
}
