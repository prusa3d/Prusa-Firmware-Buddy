/*
 * window_dlg_preheat.hpp
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#pragma once

#include "window_frame.hpp"
#include "window_text.hpp"
#include "window_list.hpp"
#include "filament.h"

struct window_dlg_preheat_t;
typedef void(dlg_on_click_cb)(window_dlg_preheat_t *);

//todo some items are most likely unused - remove them
struct window_dlg_preheat_t : public window_frame_t {
    color_t color_text;
    font_t *font;
    font_t *font_title;
    padding_ui8_t padding;
    uint32_t timer;
    uint16_t flags;
    window_text_t text;
    window_list_t list;
    window_list_item_t *filament_items;
    dlg_on_click_cb *on_click;
    string_view_utf8 caption;

    window_dlg_preheat_t();
};

extern FILAMENT_t gui_dlg_preheat(string_view_utf8 caption);
extern FILAMENT_t gui_dlg_preheat_autoselect_if_able(string_view_utf8 caption);
extern FILAMENT_t gui_dlg_preheat_forced(string_view_utf8 caption);                    //no return option
extern FILAMENT_t gui_dlg_preheat_autoselect_if_able_forced(string_view_utf8 caption); //no return option
extern int gui_dlg_list(string_view_utf8 caption, window_list_item_t *filament_items,
    dlg_on_click_cb *on_click, size_t count, int32_t ttl);
