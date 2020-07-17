/*
 * window_file_list.cpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 *  Refactoring by DRracer 2020-04-08
 */

#include "window_file_list.hpp"
#include "gui.hpp"
#include "config.h"
#include "fatfs.h"
#include "dbg.h"
#include "sound.hpp"
#include "../lang/i18n.h"
#include <algorithm>
#include "ScreenHandler.hpp"

int16_t WINDOW_CLS_FILE_LIST = 0;

void window_file_list_inc(window_file_list_t *window, int dif);
void window_file_list_dec(window_file_list_t *window, int dif);

bool window_file_list_path_is_root(const char *path) {
    return (path[0] == 0 || strcmp(path, "/") == 0);
}

/// First part of common setup/init of text rolling
/// This is called in window_file_list_inc and window_file_list_inc when the selected item changes
/// - that sometimes means the cursor stays on top or bottom and the whole window content moves
void window_file_list_init_text_roll(window_file_list_t *window) {
    window->roll.setup = TXTROLL_SETUP_INIT;
    window->roll.phase = ROLL_SETUP;
    gui_timer_restart_txtroll(window->id);
    gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, window->id);
}

void window_file_list_load(window_file_list_t *window, WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN) {
    window->ldv->ChangeDirectory(window->sfn_path,
        (sort == WF_SORT_BY_NAME) ? LDV9::SortPolicy::BY_NAME : LDV9::SortPolicy::BY_CRMOD_DATETIME,
        topSFN);
    window->count = window->ldv->TotalFilesCount();

    if (!topSFN) {
        // we didn't get any requirements about the top item
        window->index = window->count > 1 ? 1 : 0; // just avoid highlighting ".." if there is at least one file in the dir
    } else {
        if (sfnAtCursor[0] == 0) { // empty file name to start with
            window->index = 1;
        } else {
            // try to find the sfn to be highlighted
            for (window->index = 0; uint32_t(window->index) < window->ldv->VisibleFilesCount(); ++window->index) {
                if (!strcmp(sfnAtCursor, window->ldv->ShortFileNameAt(window->index).first)) {
                    break;
                }
            }
            if (window->index == int(window->ldv->VisibleFilesCount())) {
                window->index = window->count > 1 ? 1 : 0; // just avoid highlighting ".." if there is at least one file in the dir
            }
        }
    }
    window->Invalidate();
}

void window_file_set_item_index(window_file_list_t *window, int index) {
    if (window->count > index) {
        window->index = index;
        window->Invalidate();
    }
}

const char *window_file_current_LFN(window_file_list_t *window, bool *isFile) {
    auto i = window->ldv->LongFileNameAt(window->index);
    *isFile = i.second == LDV9::EntryType::FILE;
    return i.first;
}

const char *window_file_current_SFN(window_file_list_t *window, bool *isFile) {
    auto i = window->ldv->ShortFileNameAt(window->index);
    *isFile = i.second == LDV9::EntryType::FILE;
    return i.first;
}

const char *window_file_list_top_item_SFN(window_file_list_t *window) {
    return window->ldv->ShortFileNameAt(0).first;
}

void window_file_list_init(window_file_list_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->padding = padding_ui8(2, 6, 2, 6);
    window->alignment = ALIGN_LEFT_CENTER;
    window->Enable();
    window->roll.count = window->roll.px_cd = window->roll.progress = 0;
    window->roll.phase = ROLL_SETUP;
    window->roll.setup = TXTROLL_SETUP_INIT;
    gui_timer_create_txtroll(TEXT_ROLL_INITIAL_DELAY_MS, window->id);
    strlcpy(window->sfn_path, "/", FILE_PATH_MAX_LEN);

    // it is still the same address every time, no harm assigning it again.
    // Will be removed when this file gets converted to c++ (and cleaned)
    window->ldv = LDV_Get();

    display::FillRect(window->rect, window->color_back);
}

void window_file_list_done(window_file_list_t *window) {
    gui_timers_delete_by_window_id(window->id);
}

void window_file_list_draw(window_file_list_t *window) {
    int item_height = window->font->h + window->padding.top + window->padding.bottom;
    rect_ui16_t rc_win = window->rect;

    int visible_slots = rc_win.h / item_height;
    int ldv_visible_files = window->ldv->VisibleFilesCount();
    int maxi = std::min(std::min(visible_slots, ldv_visible_files), window->count);

    int i;
    for (i = 0; i < maxi; i++) {
        bool isFile = true;
        auto item = window->ldv->LongFileNameAt(i);
        isFile = item.second == LDV9::EntryType::FILE;
        if (!item.first) {
            // this should normally not happen, visible_count shall limit indices to valid items only
            continue; // ... but getting ready for the unexpected
        }
        uint16_t id_icon = isFile ? IDR_NULL : IDR_PNG_filescreen_icon_folder;

        // special handling for the link back to printing screen - i.e. ".." will be renamed to "Home"
        // and will get a nice house-like icon
        static const char home[] = N_("Home"); // @@TODO reuse from elsewhere ...
        string_view_utf8 itemText;

        if (i == 0 && strcmp(item.first, "..") == 0 && window_file_list_path_is_root(window->sfn_path)) { // @@TODO clean up, this is probably unnecessarily complex
            id_icon = IDR_PNG_filescreen_icon_home;
            itemText = string_view_utf8::MakeCPUFLASH((const uint8_t *)home);
        } else {
            // this MakeRAM is safe - render_text (below) finishes its work and the local string item.first is then no longer needed
            itemText = string_view_utf8::MakeRAM((const uint8_t *)item.first);
        }

        color_t color_text = window->color_text;
        color_t color_back = window->color_back;
        uint8_t swap = 0;

        rect_ui16_t rc = { rc_win.x, uint16_t(rc_win.y + i * item_height), rc_win.w, uint16_t(item_height) };
        padding_ui8_t padding = window->padding;

        if (rect_in_rect_ui16(rc, rc_win)) {
            if ((window->IsFocused()) && (window->index == i)) {
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

            if ((window->IsFocused()) && window->index == i) {
                if (window->roll.phase == ROLL_SETUP) { // initiation of rolling is done in functions
                    // which move cursor up or down. They can handle the situation, when the cursor
                    // stays at one place (top or bottom), but the whole window list moves up/down.
                    // Calling roll_init must be done here because of the rect.
                    // That also solves the reinit of rolling the same file name, when the cursor doesn't move.
                    roll_init(rc, itemText, window->font, padding, window->alignment, &window->roll);
                }

                render_roll_text_align(rc,
                    itemText,
                    window->font,
                    padding,
                    window->alignment,
                    color_back,
                    color_text,
                    &window->roll);

            } else {
                render_text_align(rc,
                    itemText,
                    window->font,
                    color_back, color_text,
                    padding, window->alignment);
            }

            /*	too slow
				display::DrawLine(
						point_ui16(rc_win.x, rc_win.y + (i+1) * item_height-1),
						point_ui16(rc_win.x+rc_win.w, rc_win.y + (i+1) * item_height-1),
						COLOR_GRAY);
			 */
        }
    }

    rc_win.h = rc_win.h - (i * item_height);

    if (rc_win.h) {
        rc_win.y += i * item_height;
        display::FillRect(rc_win, window->color_back);
    }
}

void window_file_list_event(window_file_list_t *window, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        Screens::Access()->DispatchEvent(window, WINDOW_EVENT_CLICK, (void *)window->index);
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
    case WINDOW_EVENT_TIMER:
        roll_text_phasing(window->id, window->font, &window->roll);
        break;
    }
}

void window_file_list_inc(window_file_list_t *window, int dif) {
    bool repaint = false;
    if (window->index >= int(window->ldv->WindowSize() - 1)) {
        repaint = window->ldv->MoveDown();
        if (!repaint) {
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
    } else {
        // this 'if' solves a situation with less files than slots on the screen
        if (window->index < int(window->ldv->TotalFilesCount() - 1)) {
            window->index += 1; // @@TODO dif > 1 pokud bude potreba;
            repaint = true;
        } else {
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
    }

    if (repaint) {
        // here we know exactly, that the selected item changed -> prepare text rolling
        window_file_list_init_text_roll(window);
        window->Invalidate();
    }
}

void window_file_list_dec(window_file_list_t *window, int dif) {
    bool repaint = false;
    if (window->index == 0) {
        // at the beginning of the window
        repaint = window->ldv->MoveUp();
        if (!repaint) {
            Sound_Play(eSOUND_TYPE_BlindAlert);
        }
    } else {
        --window->index;
        repaint = true;
    }

    if (repaint) {
        window_file_list_init_text_roll(window);
        window->Invalidate();
    }
}
