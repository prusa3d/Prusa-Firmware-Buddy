/*
 * screen_messages.cpp
 *
 *  Created on: Nov 13, 2019
 *      Author: Migi
 */

#include "screen_messages.hpp"
#include "marlin_server.hpp"
#include "ScreenHandler.hpp"
#include <stdlib.h>
#include <stdint.h>
#include "i18n.h"
#include "gui.hpp"
#include <sound.hpp>

MessageBuffer screen_messages_data_t::message_buffer;

screen_messages_data_t::screen_messages_data_t()
    : screen_t()
    , header(this)
    , footer(this)
    , term(this, GuiDefaults::RectScreenBody.TopLeft(), &term_buff) { // Rect16(10, 28, 11 * 20, 18 * 16))
    header.SetText(_("MESSAGES"));
    ClrMenuTimeoutClose();
    ClrOnSerialClose();
}

void screen_messages_data_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        Sound_Play(eSOUND_TYPE::ButtonEcho);
        Screens::Access()->Close();
        return;

    default:
        screen_t::windowEvent(sender, event, param);
        break;
    }

    // must be last window_frame_t could validate term
    char *txt = nullptr;
    while (message_buffer.try_get(txt)) {
        term.Printf("%s\n", txt);
        free(txt);
    }
}
