/*
 * window_file_list.cpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 *  Refactoring by DRracer 2020-04-08
 */
#include <algorithm>

#include "window_file_list.hpp"
#include "gui.hpp"
#include "config.h"
#include "fatfs.h"
#include "dbg.h"
#include "sound.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "cmath_ext.h"
#include "WindowMenuItems.hpp"

class FL_LABEL : public WI_LABEL_t {
public:
    FL_LABEL(string_view_utf8 label, uint16_t id_icon)
        : WI_LABEL_t(label, id_icon, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click(IWindowMenu &window_menu) {}
};

bool window_file_list_t::IsPathRoot(const char *path) {
    return (path[0] == 0 || strcmp(path, "/") == 0);
}

void window_file_list_t::Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN) {
    ldv->ChangeDirectory(sfn_path,
        (sort == WF_SORT_BY_NAME) ? LDV9::SortPolicy::BY_NAME : LDV9::SortPolicy::BY_CRMOD_DATETIME,
        topSFN);
    count = ldv->TotalFilesCount();

    if (!topSFN) {
        // we didn't get any requirements about the top item
        index = count > 1 ? 1 : 0; // just avoid highlighting ".." if there is at least one file in the dir
    } else {
        if (sfnAtCursor[0] == 0) { // empty file name to start with
            index = 1;
        } else {
            // try to find the sfn to be highlighted
            for (index = 0; uint32_t(index) < ldv->VisibleFilesCount(); ++index) {
                if (!strcmp(sfnAtCursor, ldv->ShortFileNameAt(index).first)) {
                    break;
                }
            }
            if (index == int(ldv->VisibleFilesCount())) {
                index = count > 1 ? 1 : 0; // just avoid highlighting ".." if there is at least one file in the dir
            }
        }
    }
    Invalidate();
}

void window_file_list_t::SetItemIndex(int index) {
    if (count > index && this->index != index) {
        this->index = index;
        Invalidate();
    }
}

const char *window_file_list_t::CurrentLFN(bool *isFile) {
    auto i = ldv->LongFileNameAt(index);
    *isFile = i.second == LDV9::EntryType::FILE;
    return i.first;
}

const char *window_file_list_t::CurrentSFN(bool *isFile) {
    auto i = ldv->ShortFileNameAt(index);
    *isFile = i.second == LDV9::EntryType::FILE;
    return i.first;
}

const char *window_file_list_t::TopItemSFN() {
    return ldv->ShortFileNameAt(0).first;
}

window_file_list_t::window_file_list_t(window_t *parent, Rect16 rect)
    : window_aligned_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , padding({ 2, 6, 2, 6 })
    , ldv(LDV_Get()) {
    // it is still the same address every time, no harm assigning it again.
    // Will be removed when this file gets converted to c++ (and cleaned)
    SetAlignment(ALIGN_LEFT_CENTER);
    Enable();
    strlcpy(sfn_path, "/", FILE_PATH_MAX_LEN);
}

void window_file_list_t::unconditionalDraw() {
    const int item_height = font->h + padding.top + padding.bottom;
    const int visible_slots = rect.Height() / item_height;
    const int ldv_visible_files = ldv->VisibleFilesCount();
    const int maxi = std::min(count, std::min(visible_slots, ldv_visible_files));

    int i;
    for (i = 0; i < maxi; i++) {
        auto item = ldv->LongFileNameAt(i);
        const bool isFile = item.second == LDV9::EntryType::FILE;
        if (!item.first) {
            // this should normally not happen, visible_count shall limit indices to valid items only
            continue; // ... but getting ready for the unexpected
        }
        uint16_t id_icon = isFile ? IDR_NULL : IDR_PNG_folder_full_16px;

        // special handling for the link back to printing screen - i.e. ".." will be renamed to "Home"
        // and will get a nice house-like icon
        static const char home[] = N_("Home"); // @@TODO reuse from elsewhere ...
        string_view_utf8 itemText;

        if (i == 0 && strcmp(item.first, "..") == 0 && IsPathRoot(sfn_path)) { // @@TODO clean up, this is probably unnecessarily complex
            id_icon = IDR_PNG_home_full_16px;
            itemText = string_view_utf8::MakeCPUFLASH((const uint8_t *)home);
        } else {
            // this MakeRAM is safe - render_text (below) finishes its work and the local string item.first is then no longer needed
            itemText = string_view_utf8::MakeRAM((const uint8_t *)item.first);
        }

        Rect16 rc
            = { rect.Left(), int16_t(rect.Top() + i * item_height), rect.Width(), uint16_t(item_height) };
        if (!rect.Contain(rc))
            continue;

        FL_LABEL label(itemText, id_icon);
        if ((IsFocused()) && (index == i)) {
            label.SetFocus();
            label.InitRollIfNeeded(rc);
        }
        label.Print(rc);

        // color_t color_text
        //     = this->color_text;
        // color_t color_back = COLOR_LIME;
        // uint8_t swap = 0;
        // if ((IsFocused()) && (index == i)) {
        //     SWAP(color_text, color_back);
        //     swap = ROPFN_SWAPBW;
        // }

        // padding_ui8_t padding = this->padding;
        // if (id_icon) {
        //     Rect16 icon_rc = { rc.TopLeft(), 16, 30 };
        //     rc += Rect16::Left_t(icon_rc.Width());
        //     rc -= icon_rc.Width();
        //     //            render_icon_align(icon_rc, id_icon, this->color_back, RENDER_FLG(ALIGN_CENTER, swap));
        //     render_icon_align(icon_rc, id_icon, COLOR_RED, RENDER_FLG(ALIGN_CENTER, swap));
        // } else {
        //     padding.left += 16;
        // }

        // if ((IsFocused()) && index == i) {
        //     if (roll.NeedInit()) {
        //         // there is single roll for all items, so it is reinitialized often
        //         // initiation of rolling is done in functions
        //         // which move cursor up or down. They can handle the situation, when the cursor
        //         // stays at one place (top or bottom), but the whole window list moves up/down.
        //         // Calling roll.Init must be done here because of the rect.
        //         // That also solves the reinit of rolling the same file name, when the cursor doesn't move.
        //         roll.Init(rc, itemText, font, padding, GetAlignment());
        //     }

        //     roll.RenderTextAlign(rc, itemText, font, color_back, color_text, padding, GetAlignment());

        // } else {
        //     render_text_align(rc, itemText, font, color_back, color_text, padding, GetAlignment());
        // }

        /*	too slow
				display::DrawLine(
						point_ui16(rect.x, rect.y + (i+1) * item_height-1),
						point_ui16(rect.x+rect.w, rect.y + (i+1) * item_height-1),
						COLOR_GRAY);
			 */
    }

    /// fill the rest of the window by background
    const int menu_h = i * item_height;
    Rect16 rc_win = rect;
    rc_win -= Rect16::Height_t(menu_h);
    if (rc_win.Height() <= 0)
        return;
    rc_win += Rect16::Top_t(menu_h);
    display::FillRect(rc_win, this->color_back);
}

void window_file_list_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        Screens::Access()->Get()->WindowEvent(this, GUI_event_t::CLICK, (void *)index);
        break;
    case GUI_event_t::ENC_DN:
        dec((int)param);
        break;
    case GUI_event_t::ENC_UP:
        inc((int)param);
        break;
    case GUI_event_t::CAPT_1:
        //TODO: change flag to checked
        break;
    case GUI_event_t::TEXT_ROLL:
        if (roll.Tick() == invalidate_t::yes)
            Invalidate();
        break;
    default:
        break;
    }
}

void window_file_list_t::inc(int dif) {
    if (dif == 0)
        return;

    bool repaint = false;
    bool middle = true; ///< cursor ended in the middle of the list, not at the end (or start)

    if (dif > 0) {
        while (dif-- && middle) {
            if (index >= int(ldv->WindowSize() - 1)) {
                middle = ldv->MoveDown();    ///< last result defines end of list
                repaint = repaint || middle; ///< any movement triggers repaint
            } else if (index < int(ldv->TotalFilesCount() - 1)) {
                index++;
                repaint = true;
            } else {
                middle = false;
            }
        }
    } else {
        while (dif++ && middle) {
            middle = ldv->MoveUp();      ///< last result defines end of list
            repaint = repaint || middle; ///< any movement triggers repaint
            index--;
        }
        index++;
    }

    if (!middle) {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    } else if (repaint) {
        Sound_Play(eSOUND_TYPE::EncoderMove);
    }
    if (!repaint)
        return;

    // cursor moved => rolling will be elsewhere
    roll.Deinit();
    Invalidate();

    // if (index >= int(ldv->WindowSize() - 1)) {
    //     repaint = ldv->MoveDown();
    //     if (!repaint) {
    //         Sound_Play(eSOUND_TYPE::BlindAlert);
    //     }
    // } else {
    //     // this 'if' solves a situation with less files than slots on the screen
    //     if (index < int(ldv->TotalFilesCount() - 1)) {
    //         index += 1; // @@TODO dif > 1 if needed
    //         repaint = true;
    //     } else {
    //         Sound_Play(eSOUND_TYPE::BlindAlert);
    //     }
    // }
}

void window_file_list_t::dec(int dif) {
    // inc(-dif);
    // return;

    bool repaint = false;
    if (index == 0) {
        // at the beginning of the window
        repaint = ldv->MoveUp();
        if (!repaint) {
            Sound_Play(eSOUND_TYPE::BlindAlert);
        }
    } else {
        --index;
        repaint = true;
    }

    if (repaint) {
        roll.Deinit();
        Invalidate();
        Sound_Play(eSOUND_TYPE::EncoderMove);
    }
}
