/*
 * window_dlg_preheat.hpp
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#pragma once

#include "window.hpp"
#include "window_text.hpp"
#include "window_list.hpp"
#include "filament.h"

struct window_dlg_preheat_t;
typedef void(dlg_on_click_cb)(window_dlg_preheat_t *);

extern int16_t WINDOW_CLS_DLG_PREHEAT;

//todo some items are most likely unused - remove them
struct window_dlg_preheat_t : public window_t {
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
    const char *caption;
};

struct window_class_dlg_preheat_t {
    window_class_t cls;
};

extern const window_class_dlg_preheat_t window_class_dlg_preheat;
extern FILAMENT_t gui_dlg_preheat(const char *caption);
extern FILAMENT_t gui_dlg_preheat_autoselect_if_able(const char *caption);
extern FILAMENT_t gui_dlg_preheat_forced(const char *caption);                    //no return option
extern FILAMENT_t gui_dlg_preheat_autoselect_if_able_forced(const char *caption); //no return option
extern int gui_dlg_list(const char *caption, window_list_item_t *filament_items,
    dlg_on_click_cb *on_click, size_t count, int32_t ttl);
