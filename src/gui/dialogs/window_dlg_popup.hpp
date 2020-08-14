/*
 * window_dlg_popup.hpp
 *
 *  Created on: Nov 11, 2019
 *      Author: Migi
 */

#pragma once

#include "window.hpp"
#include "marlin_server.h"

struct window_dlg_popup_t : public window_t {
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint32_t timer;
    // uint16_t flags;
    char text[MSG_MAX_LENGTH];
    window_dlg_popup_t(window_t *parent, Rect16 rect);
};

extern void gui_pop_up(void);
