/*
 * window_dlg_preheat.c
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#include "window_dlg_preheat.h"
#include "display_helper.h"
#include "gui.h"
#include "dbg.h"
#include "stm32f4xx_hal.h"
#include "filament.h"
#include "marlin_client.h"
#include "resource.h"
#include "stdlib.h"
#include "../lang/i18n.h"
#include "window_frame.h"
#include <limits>

int16_t WINDOW_CLS_DLG_PREHEAT = 0;

#define _PREHEAT_FILAMENT_CNT (FILAMENTS_END - FILAMENT_PLA)

//no return option
void window_list_filament_item_forced_cb(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    if (index <= pwindow_list->count) {
        *pptext = _(filaments[index + FILAMENT_PLA].long_name);
    } else
        *pptext = "Index ERROR";

    *pid_icon = 0;
}

void window_list_filament_item_cb(window_list_t *pwindow_list, uint16_t index,
    const char **pptext, uint16_t *pid_icon) {
    if (index == 0) {
        *pptext = "Return";
        *pid_icon = IDR_PNG_filescreen_icon_up_folder;
    } else {
        window_list_filament_item_forced_cb(pwindow_list, index - 1, pptext, pid_icon);
    }
}

void window_dlg_preheat_click_forced_cb(window_dlg_preheat_t *window) {
    FILAMENT_t index = FILAMENT_t(window->list.index + FILAMENT_PLA);
    marlin_gcode_printf("M104 S%d", (int)filaments[index].nozzle);
    marlin_gcode_printf("M140 S%d", (int)filaments[index].heatbed);
}

void window_dlg_preheat_click_cb(window_dlg_preheat_t *window) {
    if (window->list.index > 0) {
        FILAMENT_t index = FILAMENT_t(window->list.index + FILAMENT_PLA - 1);
        marlin_gcode_printf("M104 S%d", (int)filaments[index].nozzle);
        marlin_gcode_printf("M140 S%d", (int)filaments[index].heatbed);
    }
}

void window_dlg_preheat_init(window_dlg_preheat_t *window) {
    //inherit from frame
    window_class_frame.cls.init(window);
    window->win.flg |= WINDOW_FLG_ENABLED | WINDOW_FLG_INVALID;
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = gui_defaults.padding;

    int16_t id;
    rect_ui16_t rect = gui_defaults.scr_body_sz;
    if (window->caption) {
        rect.h = window->font_title->h + 2;
        id = window_create_ptr(WINDOW_CLS_TEXT, window->win.id, rect, &(window->text));
        window_set_text(id, window->caption);
        rect = gui_defaults.scr_body_sz;
        rect.y += window->font_title->h + 4;
        rect.h -= window->font_title->h + 4;
    }

    id = window_create_ptr(WINDOW_CLS_LIST, window->win.id, rect, &(window->list));
    window->list.padding = padding_ui8(20, 6, 2, 6);
    window->list.icon_rect = rect_ui16(0, 0, 16, 30);

    //window_set_item_count(id, window->filaments_count);
    window_set_item_index(id, 0);
    window_set_item_callback(id, window->filament_items);
}

void window_dlg_preheat_event(window_dlg_preheat_t *window, uint8_t event, void *param) {
    //todo, fixme, error i will not get WINDOW_EVENT_BTN_DN event here first time when it is clicked
    //but list reacts to it
    switch (event) {
    case WINDOW_EVENT_ENC_UP:
    case WINDOW_EVENT_ENC_DN: //forward up/dn events to list window
        window->list.win.cls->event(&(window->list.win), event, param);
        break;
    case WINDOW_EVENT_BTN_DN:
        if (window->timer != std::numeric_limits<uint32_t>::max()) {
            window->timer = -1; //close
            window->on_click(window);
        }
        return;
    default:
        window_frame_event((window_frame_t *)window, event, param);
    }
}

const window_class_dlg_preheat_t window_class_dlg_preheat = {
    {
        //call frame methods
        WINDOW_CLS_USER,
        sizeof(window_dlg_preheat_t),
        (window_init_t *)window_dlg_preheat_init, //must call window_frame_init inside
        (window_done_t *)window_frame_done,
        (window_draw_t *)window_frame_draw,
        (window_event_t *)window_dlg_preheat_event, //must call window_frame_event
    },
};

FILAMENT_t gui_dlg_preheat(const char *caption) {
    int ret = gui_dlg_list(
        caption,
        window_list_filament_item_cb,
        window_dlg_preheat_click_cb,
        _PREHEAT_FILAMENT_CNT + 1, //+1 back option
        -1);
    if (ret < 0)
        return FILAMENT_NONE; //timeout
    return (FILAMENT_t)ret;   //RETURN option will return FILAMENT_NONE
}

FILAMENT_t gui_dlg_preheat_autoselect_if_able(const char *caption) {
    const FILAMENT_t fil = get_filament();
    if (fil == FILAMENT_NONE) {
        //no filament selected
        return gui_dlg_preheat(caption);
    } else {
        //when filament is known, but heating is off, just turn it on and do not ask
        marlin_vars_t *p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
        if (p_vars->target_nozzle != filaments[fil].nozzle) {
            marlin_gcode_printf("M104 S%d", (int)filaments[fil].nozzle);
            marlin_gcode_printf("M140 S%d", (int)filaments[fil].heatbed);
        }
    }
    return fil;
}

//no return option
FILAMENT_t gui_dlg_preheat_forced(const char *caption) {
    int ret = gui_dlg_list(
        caption,
        window_list_filament_item_forced_cb,
        window_dlg_preheat_click_forced_cb,
        _PREHEAT_FILAMENT_CNT,
        -1 //do not leave
    );

    if (ret < 0)
        return FILAMENT_NONE;   //should not happen
    return (FILAMENT_t)(++ret); //first filament has position 0, have to change index
}

//no return option
FILAMENT_t gui_dlg_preheat_autoselect_if_able_forced(const char *caption) {
    const FILAMENT_t fil = get_filament();
    if (fil == FILAMENT_NONE) {
        //no filament selected
        return gui_dlg_preheat_forced(caption);
    } else {
        //when filament is known, but heating is off, just turn it on and do not ask
        marlin_vars_t *p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
        if (p_vars->target_nozzle != filaments[fil].nozzle) {
            marlin_gcode_printf("M104 S%d", (int)filaments[fil].nozzle);
            marlin_gcode_printf("M140 S%d", (int)filaments[fil].heatbed);
        }
    }
    return fil;
}

//returns index or -1 on timeout
//todo make this independet on preheat, in separate file
//todo caption is not showing
int gui_dlg_list(const char *caption, window_list_item_t *filament_items,
    dlg_on_click_cb *on_click, size_t count, int32_t ttl) {
    window_dlg_preheat_t dlg;
    dlg.caption = caption;
    dlg.filament_items = filament_items;
    dlg.on_click = on_click;
    //dlg.filaments_count = count;

    //parent 0 would be first screen
    //here must be -1
    int16_t id_capture = window_capture();
    int16_t id = window_create_ptr(WINDOW_CLS_DLG_PREHEAT, -1, gui_defaults.scr_body_sz, &dlg);

    window_set_item_count(dlg.list.win.id, count);

    window_t *tmp_window_1 = window_popup_ptr; //save current window_popup_ptr

    window_popup_ptr = (window_t *)&dlg;
    window_set_capture(id); //set capture to dlg, events for list are forwarded in window_dlg_preheat_event

    gui_reset_jogwheel();
    gui_invalidate();

    //window_disable(id);

    dlg.timer = HAL_GetTick();

    //ttl for ever (or very long time)
    while ((dlg.timer != std::numeric_limits<uint32_t>::max()) && ((uint32_t)(HAL_GetTick() - dlg.timer) < (uint32_t)ttl)) {
        gui_loop();
    }

    int ret;
    if (dlg.timer != std::numeric_limits<uint32_t>::max()) {
        ret = -1;
    } else {
        ret = dlg.list.index;
    }

    window_destroy(id);              //msgbox call this inside (destroys its own window)
    window_popup_ptr = tmp_window_1; //restore current window_popup_ptr
    window_invalidate(0);
    window_set_capture(id_capture);
    return ret;
}
