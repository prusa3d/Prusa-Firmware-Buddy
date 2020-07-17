/*
 * window_dlg_popup.hpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#pragma once

#include "window.hpp"
#include "marlin_server.h"

extern int16_t WINDOW_CLS_DLG_POPUP;

struct window_dlg_popup_t : public window_t {
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint32_t timer;
    uint16_t flags;
    char text[MSG_MAX_LENGTH];
};

extern void gui_pop_up(void);
