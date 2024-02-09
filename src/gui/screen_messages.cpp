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

screen_messages_data_t::screen_messages_data_t()
    : AddSuperWindow<screen_t>()
    , header(this)
    , footer(this)
    , term(this, GuiDefaults::RectScreenBody.TopLeft(), &term_buff) { // Rect16(10, 28, 11 * 20, 18 * 16))
    header.SetText(_("MESSAGES"));
    ClrMenuTimeoutClose();
    ClrOnSerialClose();
}

void screen_messages_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        Screens::Access()->Close();
        return;

    default:
        SuperWindowEvent(sender, event, param);
        break;
    }

    CircleStringBuffer<MSG_STACK_SIZE, MSG_MAX_LENGTH>::Elem elem;

    // must be last window_frame_t could validate term
    while (MsgCircleBuffer().ConsumeFirst(elem)) {
        term.Printf("%s\n", (const char *)elem);
    }
}
