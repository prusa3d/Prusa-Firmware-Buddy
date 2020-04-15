/*
 * window_dlg_preheat.h
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#ifndef WINDOW_DLG_PREHEAT_H_
#define WINDOW_DLG_PREHEAT_H_

#include "window.h"
#include "window_text.h"
#include "window_list.h"

typedef struct _window_dlg_preheat_t window_dlg_preheat_t;
typedef void(dlg_on_click_cb)(window_dlg_preheat_t *);

extern int16_t WINDOW_CLS_DLG_PREHEAT;

#pragma pack(push)
#pragma pack(1)

//todo some items are most likely unused - remove them
typedef struct _window_dlg_preheat_t {
    window_t win;
    color_t color_back;
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
} window_dlg_preheat_t;

typedef struct _window_class_dlg_preheat_t {
    window_class_t cls;
} window_class_dlg_preheat_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const window_class_dlg_preheat_t window_class_dlg_preheat;
extern int gui_dlg_preheat(const char *caption);
extern int gui_dlg_preheat_autoselect_if_able(const char *caption);
extern int gui_dlg_preheat_forced(const char *caption);                    //no return option
extern int gui_dlg_preheat_autoselect_if_able_forced(const char *caption); //no return option
extern int gui_dlg_list(const char *caption, window_list_item_t *filament_items,
    dlg_on_click_cb *on_click, size_t count, int32_t ttl);
#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* WINDOW_DLG_PREHEAT_H_ */
