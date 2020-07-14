/*
 * window_header.hpp
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include "gui.hpp"
#include "marlin_events.h"
#include "window_text.hpp"

typedef enum {
    HEADER_ISTATE_OFF,
    HEADER_ISTATE_ON,
    HEADER_ISTATE_ACTIVE
} header_states_t;

typedef enum {
    HEADER_ICON_USB, // must be first!
    HEADER_ICON_LAN,
    HEADER_ICON_WIFI
    // for next icon, update HEADER_ICON_COUNT define !
} header_icons_t;

#define HEADER_ICON_COUNT HEADER_ICON_WIFI + 1

struct window_class_header_t {
    window_class_t cls;
};

struct window_header_t : public window_text_t {
    uint16_t id_res;
    header_states_t icons[HEADER_ICON_COUNT]; // usb, lan, wifi
    // char time[10];
};

extern int16_t WINDOW_CLS_HEADER;

extern const window_class_header_t window_class_header;

void p_window_header_set_icon(window_header_t *window, uint16_t id_res);

void p_window_header_icon_off(window_header_t *window, header_icons_t icon);
void p_window_header_icon_on(window_header_t *window, header_icons_t icon);
void p_window_header_icon_active(window_header_t *window, header_icons_t icon);

header_states_t p_window_header_get_state(window_header_t *window,
    header_icons_t icon);
void p_window_header_set_text(window_header_t *window, string_view_utf8 text);
int p_window_header_event_clr(window_header_t *window, MARLIN_EVT_t evt_id);

#define window_header_events(window)                             \
    p_window_header_event_clr(window, MARLIN_EVT_MediaInserted); \
    p_window_header_event_clr(window, MARLIN_EVT_MediaRemoved);  \
    p_window_header_event_clr(window, MARLIN_EVT_MediaError);
