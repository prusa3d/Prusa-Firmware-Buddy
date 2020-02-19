/*
 * screen_messages.c
 *
 *  Created on: Nov 13, 2019
 *      Author: Migi
 */

#include "gui.h"
#include "screen_menu.h"
#include "marlin_server.h"
#include <stdlib.h>

#pragma pack(push)
#pragma pack(1)

typedef struct _screen_messages_data_t {
    window_frame_t root;
    window_header_t header;
    window_list_t list;

    status_footer_t *pfooter;
} screen_messages_data_t;

#pragma pack(pop)

extern screen_t screen_messages;
extern msg_stack_t msg_stack;

#define pmsg ((screen_messages_data_t *)screen->pdata)

void _window_list_add_message_item(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *msg_icon) {
    static const char empty_str[] = "";
    static const char back_str[] = "BACK";
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
    for (uint8_t i = del_index; i + 1 < msg_stack.count; i++)
        strncpy(msg_stack.msg_data[i], msg_stack.msg_data[i + 1], MSG_MAX_LENGTH);
    msg_stack.count--;
}

void screen_messages_init(screen_t *screen) {

    int16_t id;
    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pmsg->root));
    window_disable(root);

    id = window_create_ptr(WINDOW_CLS_HEADER, root, rect_ui16(0, 0, 240, 31), &(pmsg->header));
    // p_window_header_set_icon(&(pmsg->header), IDR_PNG_status_icon_menu);					ICONka od Michala Fanty
    p_window_header_set_text(&(pmsg->header), "MESSAGES");

    id = window_create_ptr(WINDOW_CLS_LIST, root, rect_ui16(0, 32, 240, 320 - 83), &(pmsg->list));
    window_set_item_count(id, msg_stack.count + 1);
    window_set_item_index(id, 0);
    window_set_item_callback(id, _window_list_add_message_item);

    window_set_capture(pmsg->list.win.id);

    pmsg->pfooter = (status_footer_t *)gui_malloc(sizeof(status_footer_t));
    status_footer_init(pmsg->pfooter, root);
}

void screen_messages_draw(screen_t *screen) {
}

int screen_messages_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {

    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    case WINDOW_EVENT_CLICK:
        if (pmsg->list.index == 0) {
            screen_close();
            return 1;
        } /*else if (pmsg->list.index <= msg_stack.count) {		TODO: Deleted message stays on the screen
			_msg_stack_del(pmsg->list.index - 1);
			_window_invalidate((window_t*)&(pmsg->list));
		} */
        break;
    default:
        break;
    }

    pmsg->list.count = msg_stack.count + 1;

    status_footer_event(pmsg->pfooter, window, event, param);

    return 0;
}

void screen_messages_done(screen_t *screen) {
    window_destroy(pmsg->root.win.id);
    free(pmsg->pfooter);
}

screen_t screen_messages = {
    0,
    0,
    screen_messages_init,
    screen_messages_done,
    screen_messages_draw,
    screen_messages_event,
    sizeof(screen_messages_data_t), //data_size
    0,                              //pdata
};

const screen_t *pscreen_messages = &screen_messages;
