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

void _window_list_add_message_item(window_list_t * /*pwindow_list*/, uint16_t index,
    const char **pptext, uint16_t *msg_icon) {
    static const char empty_str[] = "";
    static const char back_str[] = N_("BACK");
    if (index == 0) {
        *pptext = back_str;
        //*pid_icon = IDR_PNG_filescreen_icon_up_folder;
    } else {
        if (index <= msg_stack.count)
            *pptext = msg_stack.msg_data[index - 1];
        else
            *pptext = empty_str; // it shouldn't ever get here (safety measure)
    }
    *msg_icon = 0;
}

void _msg_stack_del(uint8_t del_index) { // del_index = < 0 ; MSG_STACK_SIZE - 1 >

    // when we delete from last spot of the limited stack [MSG_STACK_SIZE - 1], no swapping is needed, for cycle won't start
    for (uint8_t i = del_index; i + 1 < msg_stack.count; i++) {
        memset(msg_stack.msg_data[i], '\0', sizeof(msg_stack.msg_data[i]) * sizeof(char)); // set to zeros to be on the safe side
        strlcpy(msg_stack.msg_data[i], msg_stack.msg_data[i + 1], sizeof(msg_stack.msg_data[i]));
    }
    msg_stack.count--;
}

screen_messages_data_t::screen_messages_data_t()
    : window_frame_t()
    , header(this)
    , footer(this)
    , list(this, GuiDefaults::RectScreenBody) {
    Disable();
    header.SetText(_("MESSAGES"));

    list.SetItemCount(msg_stack.count + 1);
    list.SetItemIndex(0);
    list.SetCallback(_window_list_add_message_item);

    list.SetCapture();
}

void screen_messages_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {

    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    case WINDOW_EVENT_CLICK:
        if (list.index == 0) {
            Screens::Access()->Close();
            return;
        }
        break;
    default:
        break;
    }

    list.count = msg_stack.count + 1;
}
