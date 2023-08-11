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
#include "sound.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "cmath_ext.h"
#include "gui_invalidate.hpp"
#include "img_resources.hpp"
#include <config_store/store_instance.hpp>
#if _DEBUG
    #include "bsod.h"
#endif

GuiFileSort::GuiFileSort() {
    sort = static_cast<WF_Sort_t>(config_store().file_sort.get());
}

GuiFileSort &GuiFileSort::instance() {
    static GuiFileSort ret;
    return ret;
}

WF_Sort_t GuiFileSort::Get() {
    return instance().sort;
}

void GuiFileSort::Set(WF_Sort_t val) {
    if (instance().sort == val)
        return;

    config_store().file_sort.set(static_cast<uint8_t>(val));
    instance().sort = val;
}

// static definitions
char *window_file_list_t::root = nullptr;

bool window_file_list_t::IsPathRoot(const char *path) {
    return (path[0] == 0 || (root && strcmp(path, root) == 0));
}

void window_file_list_t::Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN) {
    ldv.ChangeDirectory(sfn_path,
        (sort == WF_SORT_BY_NAME) ? LDV::SortPolicy::BY_NAME : LDV::SortPolicy::BY_CRMOD_DATETIME,
        topSFN);
    count = ldv.TotalFilesCount();

    if (!topSFN) {
        // we didn't get any requirements about the top item
        index = count > 1 ? 1 : 0; // just avoid highlighting ".." if there is at least one file in the dir
    } else {
        if (sfnAtCursor[0] == 0) { // empty file name to start with
            index = 1;
        } else {
            // try to find the sfn to be highlighted
            for (index = 0; uint32_t(index) < ldv.VisibleFilesCount(); ++index) {
                if (!strcmp(sfnAtCursor, ldv.ShortFileNameAt(index).first)) {
                    break;
                }
            }
            if (index == int(ldv.VisibleFilesCount())) {
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

const char *window_file_list_t::CurrentLFN(bool *isFile) const {
    auto i = ldv.LongFileNameAt(index);
    if (isFile)
        *isFile = i.second == LDV::EntryType::FILE;
    return i.first;
}

const char *window_file_list_t::CurrentSFN(bool *isFile) const {
    auto i = ldv.ShortFileNameAt(index);
    if (isFile)
        *isFile = i.second == LDV::EntryType::FILE;
    return i.first;
}

const char *window_file_list_t::TopItemSFN() {
    return ldv.ShortFileNameAt(0).first;
}

window_file_list_t::window_file_list_t(window_t *parent, Rect16 rc)
    : AddSuperWindow<window_aligned_t>(parent, rc = Rect16::Height_t(item_height * LazyDirViewSize))
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , entire_window_invalid(true)
    , activeItem(string_view_utf8(), nullptr) {
    DisableLongHoldScreenAction();
    SetAlignment(Align_t::LeftCenter());
    Enable();
    strlcpy(sfn_path, "/usb", FILE_PATH_BUFFER_LEN);
}

void window_file_list_t::unconditionalDraw() {
    if (entire_window_invalid) {
        super::unconditionalDraw();
    }

    const int visible_slots = LazyDirViewSize;
    const int ldv_visible_files = ldv.VisibleFilesCount();
    const int maxi = std::min(count, std::min(visible_slots, ldv_visible_files));

    for (int i = 0; i < maxi; i++) {
        if (valid_items[i])
            continue;

        // Return item
        if (is_return_item(i)) {
            return_item.Print(itemRect(i));
            continue;
        }

        // Focused item, scrolling text
        if (IsFocused() && index == i) {
            activeItem.Print(itemRect(i));
            continue;
        }

        // General list item
        FL_LABEL label(itemText(i), itemIcon(i));
        label.Print(itemRect(i));
    }

    // no need fill the rest of the window with background, since there is no rest of the window
}

void window_file_list_t::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        Screens::Access()->Get()->WindowEvent(this, GUI_event_t::CLICK, (void *)index);
        break;
    case GUI_event_t::ENC_DN:
        inc(-(int)param);
        break;
    case GUI_event_t::ENC_UP:
        inc((int)param);
        break;
    case GUI_event_t::FOCUS1: // focus set
    case GUI_event_t::CAPT_1: // capture set
        selectNewItem();
        break;
    case GUI_event_t::TEXT_ROLL:
        if (IsFocused()) {
            activeItem.Roll();
            if (activeItem.IsInvalid()) {
                invalidateItem(index);
            }
        }
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
    int old_index = index;
    if (dif > 0) {
        while (dif-- && middle) {
            if (index >= int(ldv.WindowSize() - 1)) {
                middle = ldv.MoveDown();     ///< last result defines end of list
                repaint = repaint || middle; ///< any movement triggers repaint
            } else if (index < int(ldv.TotalFilesCount() - 1)) {
                ++index;
            } else {
                middle = false;
            }
        }
    } else {
        while (dif++ && middle) {
            if (index <= 0) {
                middle = ldv.MoveUp();       ///< last result defines end of list
                repaint = repaint || middle; ///< any movement triggers repaint
            } else {
                --index;
            }
        }
    }

    if (!middle) {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    } else {
        Sound_Play(eSOUND_TYPE::EncoderMove);
    }

    if (!repaint) {
        if (index != old_index) {
            invalidateItem(index);
            invalidateItem(old_index);
            selectNewItem();
        }
        return;
    }

    // can not use Invalidate, it would cause redraw of background
    valid_items.fill(false);
    activeItem.clrFocus();
    selectNewItem();
    super::invalidate(GetRect());
}

/**
 * @brief ugly copy od inc method with some modifications
 * TODO make window_file_list_t child of
 *
 * @param dif
 */
void window_file_list_t::roll_screen(int dif) {
    if (dif == 0)
        return;

    bool repaint = false;
    bool middle = true; ///< cursor ended in the middle of the list, not at the end (or start)
    int old_index = index;
    if (dif > 0) {
        while (dif-- && middle) {
            middle = ldv.MoveDown();     ///< last result defines end of list
            repaint = repaint || middle; ///< any movement triggers repaint
        }
    } else {
        while (dif++ && middle) {
            middle = ldv.MoveUp();       ///< last result defines end of list
            repaint = repaint || middle; ///< any movement triggers repaint
        }
    }

    if (!middle) {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    } else {
        Sound_Play(eSOUND_TYPE::EncoderMove);
    }

    if (!repaint) {
        if (index != old_index) {
            invalidateItem(index);
            invalidateItem(old_index);
            selectNewItem();
        }
        return;
    }

    // can not use Invalidate, it would cause redraw of background
    valid_items.fill(false);
    activeItem.clrFocus();
    selectNewItem();
    super::invalidate(GetRect());
}

void window_file_list_t::selectNewItem() {
    if (!IsFocused())
        return;

    const int visible_slots = LazyDirViewSize;
    const int ldv_visible_files = ldv.VisibleFilesCount();
    const int maxi = std::min(count, std::min(visible_slots, ldv_visible_files));

    if (index >= 0 && index < maxi) {
        activeItem.setFocus();
        activeItem.SetLabel(itemText(index));
        activeItem.SetIconId(itemIcon(index));
        activeItem.InitRollIfNeeded(itemRect(index));
    }

    if (is_return_item(index)) {
        return_item.setFocus();
    } else {
        return_item.clrFocus();
    }
}

void window_file_list_t::SetRoot(char *rootPath) {
    root = rootPath;
}

void window_file_list_t::invalidate(Rect16 validation_rect) {
    valid_items.fill(false); // TODO respect validation_rect
    entire_window_invalid = true;
    activeItem.clrFocus();
    selectNewItem();
    activeItem.Roll(); // first call causes additional invalidation, it does not matter here, but would flicker in case it was not called
    super::invalidate(validation_rect);
}

void window_file_list_t::invalidateItem(int index) {
    valid_items[index] = false;
    super::invalidate(GetRect());
    gui_invalidate();
}

void window_file_list_t::validate(Rect16 validation_rect) {
    valid_items.fill(true); // TODO respect validation_rect
    super::validate(validation_rect);
    entire_window_invalid = false;
}

Rect16 window_file_list_t::itemRect(int index) const {
    const Rect16 rc = { Left(), Rect16::Top_t(Top() + index * item_height), Width(), item_height };
    return GetRect().Intersection(rc);
}

const img::Resource *window_file_list_t::itemIcon(int index) const {
    auto item = ldv.LongFileNameAt(index);
    const bool isFile = item.second == LDV::EntryType::FILE;
    if (!item.first) {
        // this should normally not happen, visible_count shall limit indices to valid items only
        return nullptr; // ... but getting ready for the unexpected
    }
    return isFile ? nullptr : &img::folder_full_16x16;
}

// special handling for the link back to printing screen - i.e. ".." will be renamed to "Home"
// and will get a nice house-like icon
string_view_utf8 window_file_list_t::itemText(int index) const {
    string_view_utf8 itemText;
    auto item = ldv.LongFileNameAt(index);

    // this MakeRAM is safe - render_text (below) finishes its work and the local string item.first is then no longer needed
    return string_view_utf8::MakeRAM((const uint8_t *)item.first);
}

void window_file_list_t::RollUp() {
    roll_screen(1 - int(LazyDirViewSize));
}

void window_file_list_t::RollDown() {
    roll_screen(LazyDirViewSize - 1);
}

bool window_file_list_t::is_return_item(const int index) const {
    const auto item = ldv.LongFileNameAt(index);
    return index == 0 && IsPathRoot(sfn_path) && strcmp(item.first, "..") == 0;
}
