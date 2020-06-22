/*
 * window_header.h
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include "gui.h"
#include "marlin_events.h"

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

typedef struct _window_class_header_t {
    window_class_t cls;
} window_class_header_t;

typedef struct _window_header_t {
    window_t win;

    color_t color_back;
    color_t color_text;
    font_t *font;
    padding_ui8_t padding;
    uint8_t alignment;
    uint16_t id_res;
    header_states_t icons[HEADER_ICON_COUNT]; // usb, lan, wifi
    const char *label;

    // char time[10];
} window_header_t;

extern int16_t WINDOW_CLS_HEADER;

extern const window_class_header_t window_class_header;

void p_window_header_set_icon(window_header_t *window, uint16_t id_res);

void p_window_header_icon_off(window_header_t *window, header_icons_t icon);
void p_window_header_icon_on(window_header_t *window, header_icons_t icon);
void p_window_header_icon_active(window_header_t *window, header_icons_t icon);

header_states_t p_window_header_get_state(window_header_t *window,
    header_icons_t icon);
void p_window_header_set_text(window_header_t *window, const char *text);
int p_window_header_event_clr(window_header_t *window, MARLIN_EVT_t evt_id);

#define window_header_events(window)                             \
    p_window_header_event_clr(window, MARLIN_EVT_MediaInserted); \
    p_window_header_event_clr(window, MARLIN_EVT_MediaRemoved);  \
    p_window_header_event_clr(window, MARLIN_EVT_MediaError);
