/*
 * window_file_list.c
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#include "window_file_list.h"
#include "gui.h"
#include "config.h"
#include "fatfs.h"
//#include "usb_host.h"
#include "dbg.h"

//extern ApplicationTypeDef Appli_state;

int pattern_matching( /* 0:not matched, 1:matched */
    const TCHAR *pat, /* Matching pattern */
    const TCHAR *nam, /* String to be tested */
    int skip,         /* Number of pre-skip chars (number of ?s) */
    int inf           /* Infinite search (* specified) */
);

int16_t WINDOW_CLS_FILE_LIST = 0;

void window_file_list_inc(window_file_list_t *window, int dif);
void window_file_list_dec(window_file_list_t *window, int dif);

//dir > file
int file_item_cmp_by_is_dir(const FILINFO *ia, const FILINFO *ib) {
    return (ib->fattrib & AM_DIR) - (ia->fattrib & AM_DIR); // directory (1) first
}

int file_item_cmp_by_name(const void *a, const void *b) {
    const FILINFO *ia = (FILINFO *)a;
    const FILINFO *ib = (FILINFO *)b;

    int rv = file_item_cmp_by_is_dir(ia, ib);
    if (rv)
        return rv;

    rv = strcmp(ia->fname, ib->fname);
    return rv;
}

int file_item_cmp_by_time(const void *a, const void *b) {
    FILINFO *ia = (FILINFO *)a;
    FILINFO *ib = (FILINFO *)b;

    int rv = file_item_cmp_by_is_dir(ia, ib);
    if (rv)
        return rv;

    if (ia->fdate != ib->fdate) {
        return ib->fdate - ia->fdate;
    }
    if (ia->ftime != ib->ftime) {
        return ib->ftime - ia->ftime;
    }
    return file_item_cmp_by_name(a, b);
}

void window_file_list_load(window_file_list_t *window, const char **filters, size_t filters_cnt,
    WF_Sort_t sort) {
    DIR dir;
    FRESULT fres;
    FILINFO info;
    int i = 0;

    memset(window->file_items, '\0', SDSORT_LIMIT * sizeof(FILINFO));

    strcpy(window->file_items[i].fname, "..");
    window->file_items[i].fattrib |= AM_DIR;
    window->file_items[i].ftime = UINT16_MAX;
    window->file_items[i++].fdate = UINT16_MAX;

    fres = f_findfirst(&dir, &info, window->altpath, "*");
    if (fres != FR_OK) {
        _dbg("Findfirst error: %d", fres);
    }

    int pattern_match;
    while (fres == FR_OK && info.fname[0] && i < SDSORT_LIMIT) { /* Repeat while an item is found */
        pattern_match = info.fattrib & AM_DIR;                   //print directory
        for (size_t filt_no = 0; (!pattern_match) && (filt_no < filters_cnt); ++filt_no) {
            if (pattern_matching(filters[filt_no], info.fname, 0, 0))
                pattern_match = 1;
        }

        if (info.fattrib & AM_SYS)
            pattern_match = 0; //skip sys files
        if (info.fattrib & AM_HID)
            pattern_match = 0; //skip hidden files

        if (pattern_match) {
            window->file_items[i++] = info;
        }

        fres = f_findnext(&dir, &info);
    }
    if (i == SDSORT_LIMIT) {
        // TODO: Notification
    }

    f_closedir(&dir);
    if (sort == WF_SORT_BY_NAME) {
        qsort(window->file_items, i, sizeof(window->file_items[0]), file_item_cmp_by_name);
    } else {
        qsort(window->file_items, i, sizeof(window->file_items[0]), file_item_cmp_by_time);
    }

    window->count = i;
    window->index = 0;
    window->top_index = 0;
    _window_invalidate((window_t *)window);
}

void window_file_set_item_index(window_file_list_t *window, int index) {
    if (window->count > index) {
        window->index = index;
        _window_invalidate((window_t *)window);
    }
}

void window_file_list_init(window_file_list_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->padding = padding_ui8(2, 6, 2, 6);
    window->alignment = ALIGN_LEFT_CENTER;
    window->win.flg |= WINDOW_FLG_ENABLED;

    strcpy(window->altpath, "/");
}

void window_file_list_done(window_file_list_t *window) {}

void window_file_list_draw(window_file_list_t *window) {
    int item_height = window->font->h + window->padding.top + window->padding.bottom;
    rect_ui16_t rc_win = window->win.rect;

    int visible_count = rc_win.h / item_height;
    int i;
    for (i = 0; i < visible_count && i < window->count; i++) {
        int idx = i + window->top_index;
        uint16_t id_icon = IDR_NULL;
        if (idx == 0) {
            id_icon = (!strcmp(window->altpath, "/")) ? IDR_PNG_filescreen_icon_home : IDR_PNG_filescreen_icon_up_folder;
        } else if (window->file_items[idx].fattrib & AM_DIR) {
            id_icon = IDR_PNG_filescreen_icon_folder;
        }

        color_t color_text = window->color_text;
        color_t color_back = window->color_back;
        uint8_t swap = 0;

        rect_ui16_t rc = { rc_win.x, rc_win.y + i * item_height,
            rc_win.w, item_height };
        padding_ui8_t padding = window->padding;

        if (rect_in_rect_ui16(rc, rc_win)) {
            /* TODO: disabled file types
			if (window->file_items[idx].flg & WF_DISABLED)
			{
				color_text = window->color_disabled;
			}
			*/

            if ((window->win.flg & WINDOW_FLG_FOCUSED) && (window->index == idx)) {
                color_t swp = color_text;
                color_text = color_back;
                color_back = swp;
                swap = ROPFN_SWAPBW;
            }

            if (id_icon) {
                rect_ui16_t irc = { rc.x, rc.y, 16, 30 };
                rc.x += irc.w;
                rc.w -= irc.w;
                render_icon_align(irc, id_icon,
                    window->color_back, RENDER_FLG(ALIGN_CENTER, swap));
            } else {
                padding.left += 16;
            }

            render_text_align(rc, window->file_items[idx].fname, window->font,
                color_back, color_text,
                padding, window->alignment);

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
    int item_height = window->font->h + window->padding.top + window->padding.bottom;
    int visible_count = window->win.rect.h / item_height;
    int old = window->index;
    window->index += dif;
    if (window->index >= window->count)
        window->index = window->count - 1;
    if (window->index >= (window->top_index + visible_count))
        window->top_index = window->index - visible_count + 1;

    if (window->index != old)
        _window_invalidate((window_t *)window);
}

void window_file_list_dec(window_file_list_t *window, int dif) {
    int old = window->index;
    window->index -= dif;
    if (window->index < 0)
        window->index = 0;
    if (window->index < window->top_index)
        window->top_index = window->index;

    if (window->index != old)
        _window_invalidate((window_t *)window);
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
