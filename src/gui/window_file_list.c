/*
 * window_file_list.c
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 *  Refactoring by DRracer 2020-04-08
 */

#include "window_file_list.h"
#include "gui.h"
#include "config.h"
#include "fatfs.h"
//#include "usb_host.h"
#include "dbg.h"
#include "lazyfilelist-c-api.h"

//extern ApplicationTypeDef Appli_state;

int16_t WINDOW_CLS_FILE_LIST = 0;

void window_file_list_inc(window_file_list_t *window, int dif);
void window_file_list_dec(window_file_list_t *window, int dif);

void window_file_list_load(window_file_list_t *window, WF_Sort_t sort) {
    if (!LDV_ChangeDir(window->ldv, sort == WF_SORT_BY_NAME, window->altpath)) {
        _dbg("LDV_ChangeDir error");
    }

    window->count = LDV_TotalFilesCount(window->ldv);
    window->index = 0;
    _window_invalidate((window_t *)window);
}

void window_file_set_item_index(window_file_list_t *window, int index) {
    if (window->count > index) {
        window->index = index;
        _window_invalidate((window_t *)window);
    }
}

const char *window_file_current_fname(window_file_list_t *window, bool *isFile) {
    return LDV_FileAt(window->ldv, window->index, isFile);
}

void window_file_list_init(window_file_list_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->padding = padding_ui8(2, 6, 2, 6);
    window->alignment = ALIGN_LEFT_CENTER;
    window->win.flg |= WINDOW_FLG_ENABLED;

    strcpy(window->altpath, "/");

    // it is still the same address every time, no harm assigning it again.
    // Will be removed when this file gets converted to c++ (and cleaned)
    window->ldv = LDV_Get();
}

void window_file_list_done(window_file_list_t *window) {}

void window_file_list_draw(window_file_list_t *window) {
    int item_height = window->font->h + window->padding.top + window->padding.bottom;
    rect_ui16_t rc_win = window->win.rect;

    int visible_count = rc_win.h / item_height;
    int i;
    for (i = 0; i < visible_count && i < window->count; i++) {
        bool isFile = true;
        const char *item = LDV_FileAt(window->ldv, i, &isFile);
        if (!item) {
            // this should normally not happen, visible_count shall limit indices to valid items only
            continue; // ... but getting ready for the unexpected
        }
        uint16_t id_icon = isFile ? IDR_NULL : IDR_PNG_filescreen_icon_folder;

        // special handling for the link back to printing screen - i.e. ".." will be renamed to "Home"
        // and will get a nice house-like icon
        static const char home[] = "Home";                                            // @@TODO reuse from elsewhere ...
        if (i == 0 && strcmp(item, "..") == 0 && strcmp(window->altpath, "/") == 0) { // @@TODO clean up, this is probably unnecessarily complex
            id_icon = IDR_PNG_filescreen_icon_home;
            item = home;
        }

        color_t color_text = window->color_text;
        color_t color_back = window->color_back;
        uint8_t swap = 0;

        rect_ui16_t rc = { rc_win.x, rc_win.y + i * item_height, rc_win.w, item_height };
        padding_ui8_t padding = window->padding;

        if (rect_in_rect_ui16(rc, rc_win)) {
            if ((window->win.flg & WINDOW_FLG_FOCUSED) && (window->index == i)) {
                color_t swp = color_text;
                color_text = color_back;
                color_back = swp;
                swap = ROPFN_SWAPBW;
            }

            if (id_icon) {
                rect_ui16_t irc = { rc.x, rc.y, 16, 30 };
                rc.x += irc.w;
                rc.w -= irc.w;
                render_icon_align(irc, id_icon, window->color_back, RENDER_FLG(ALIGN_CENTER, swap));
            } else {
                padding.left += 16;
            }

            render_text_align(rc, item, window->font, color_back, color_text, padding, window->alignment);

            /*	too slow
				display->draw_line(
						point_ui16(rc_win.x, rc_win.y + (i+1) * item_height-1),
						point_ui16(rc_win.x+rc_win.w, rc_win.y + (i+1) * item_height-1),
						COLOR_GRAY);
			 */
        }
    }

    rc_win.h = rc_win.h - (i * item_height);

    if (rc_win.h) {
        rc_win.y += i * item_height;
        display->fill_rect(rc_win, window->color_back);
    }
}

void window_file_list_event(window_file_list_t *window, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        screen_dispatch_event(NULL, WINDOW_EVENT_CLICK, (void *)window->index);
        break;
    case WINDOW_EVENT_ENC_DN:
        window_file_list_dec(window, (int)param);
        break;
    case WINDOW_EVENT_ENC_UP:
        window_file_list_inc(window, (int)param);
        break;
    case WINDOW_EVENT_CAPT_1:
        //TODO: change flag to checked
        break;
    }
}

void window_file_list_inc(window_file_list_t *window, int dif) {
    bool repaint = false;
    if (window->index >= LDV_VisibleFilesCount(window->ldv) - 1) {
        repaint = LDV_MoveDown(window->ldv);
    } else {
        // this 'if' solves a situation with less files than slots on the screen
        if (window->index < LDV_TotalFilesCount(window->ldv) - 1) {
            window->index += 1; // @@TODO dif > 1 pokud bude potreba;
            repaint = true;
        }
    }

    if (repaint) {
        _window_invalidate((window_t *)window);
    }
}

void window_file_list_dec(window_file_list_t *window, int dif) {
    bool repaint = false;
    if (window->index == 0) {
        // at the beginning of the window
        repaint = LDV_MoveUp(window->ldv);
    } else {
        --window->index;
        repaint = true;
    }

    if (repaint) {
        _window_invalidate((window_t *)window);
    }
}

const window_class_file_list_t window_class_file_list = {
    {
        WINDOW_CLS_USER,
        sizeof(window_file_list_t),
        (window_init_t *)window_file_list_init,
        (window_done_t *)window_file_list_done,
        (window_draw_t *)window_file_list_draw,
        (window_event_t *)window_file_list_event,
    },
};
