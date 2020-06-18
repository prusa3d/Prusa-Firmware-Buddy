/*
 * window_dlg_popup.h
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#pragma once

#include "window.h"
#include "marlin_server.h"

typedef struct _window_dlg_popup_t window_dlg_popup_t;

extern int16_t WINDOW_CLS_DLG_POPUP;

typedef struct _window_dlg_popup_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint32_t timer;
    uint16_t flags;
    char text[MSG_MAX_LENGTH];

} window_dlg_popup_t;

typedef struct _window_class_dlg_popup_t {
    window_class_t cls;
} window_class_dlg_popup_t;

extern const window_class_dlg_popup_t window_class_dlg_popup;
extern void gui_pop_up(void);
