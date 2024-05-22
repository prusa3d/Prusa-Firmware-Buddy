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
    if (instance().sort == val) {
        return;
    }

    config_store().file_sort.set(static_cast<uint8_t>(val));
    instance().sort = val;
}

// static definitions
char *window_file_list_t::root = nullptr;

void window_file_list_t::set_scroll_offset(int set) {
    if (scroll_offset() == set) {
        return;
    }

    IWindowMenu::set_scroll_offset(set);

    // -1 because ldv counts ".." as index -1 :/
    [[maybe_unused]] const auto new_window_offset = ldv.set_window_offset(set - 1);
    assert(new_window_offset == set - 1);

    invalidate_all_slots();
}

std::optional<int> window_file_list_t::focused_item_index() const {
    return focused_index_;
}

bool window_file_list_t::move_focus_to_index(std::optional<int> target_index) {
    if (focused_index_ == target_index) {
        return true;
    }

    auto focused_slot_opt = this->focused_slot();
    if (focused_slot_opt) {
        invalidate_slot(focused_slot_opt);
        focused_item_delegate.roll.Deinit();
    }

    focused_index_ = target_index;
    if (!target_index) {
        IWindowMenuItem::move_focus(nullptr);
        return true;
    }

    ensure_item_on_screen(target_index);

    // The focus we're setting must be in the visible range, otherwise we wouldn't be able to access its properties (itemText, ...)
    assert(this->focused_slot());

    const auto focused_slot = *this->focused_slot();
    invalidate_slot(focused_slot);

    if (is_return_slot(focused_slot)) {
        return_item_delegate.move_focus();
    }

    else {
        focused_item_delegate.move_focus();
        focused_item_delegate.SetLabel(itemText(focused_slot));
        focused_item_delegate.SetIconId(itemIcon(focused_slot));
        focused_item_delegate.InitRollIfNeeded(slot_rect(focused_slot));
    }

    return true;
}

bool window_file_list_t::IsPathRoot(const char *path) {
    return (path[0] == 0 || (root && strcmp(path, root) == 0));
}

void window_file_list_t::Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN) {
    ldv.ChangeDirectory(
        sfn_path,
        (sort == WF_SORT_BY_NAME) ? LDV::SortPolicy::BY_NAME : LDV::SortPolicy::BY_CRMOD_DATETIME,
        topSFN ?: sfnAtCursor);

    item_count_ = ldv.TotalFilesCount();

    // Now, ldv has adjusted its window offset and we need to synchronize scroll_offset with it properly.
    {
        // +1 because ldv counts ".." as index -1 :/
        int target_scroll_offset = std::min(ldv.window_offset() + 1, max_scroll_offset());

        // If !topSFN -> we're trying to just focus sfnAtCursor.
        // In this case, it doesn't necessarily have to be on the top and we can be a bit smarter.
        // If the item is within the first visible window, we won't scroll on it, keep the scroll offset on 0
        if (!topSFN && target_scroll_offset < max_items_on_screen_count()) {
            target_scroll_offset = 0;
        }

        // We might have changed the target_scroll_offset, so update ldv to match it
        ldv.set_window_offset(target_scroll_offset - 1);

        // And hard set the scroll offset for the menu itself
        IWindowMenu::set_scroll_offset(target_scroll_offset);
    }

    // Focus item if appropriate
    {
        std::optional<int> target_focused_index;

        // If we've been given sfnAtCursor, try looking it up in the ldv window
        if (sfnAtCursor && sfnAtCursor[0]) {
            for (const auto &rec : ldv.data()) {
                if (!strcmp(sfnAtCursor, rec.sfn)) {
                    target_focused_index = scroll_offset() + (&rec - ldv.data().data());
                    break;
                }
            }
        }

        if (!target_focused_index && should_focus_item_on_init()) {
            // Try avoid highlighting ".." if there's any file in the dir
            target_focused_index = (item_count() > 1) ? 1 : 0;
        }

        // Force focused index update
        focused_index_ = std::nullopt;

        move_focus_to_index(target_focused_index);
    }

    Invalidate();
}

const char *window_file_list_t::CurrentLFN(bool *isFile) const {
    const auto focused_slot = this->focused_slot();
    assert(focused_slot);

    auto i = ldv.LongFileNameAt(*focused_slot);
    if (isFile) {
        *isFile = (i.second == LDV::EntryType::FILE);
    }

    return i.first;
}

const char *window_file_list_t::CurrentSFN(bool *isFile) const {
    const char *result = nullptr;
    bool is_file = false;

    if (const auto focused_slot = this->focused_slot()) {
        auto i = ldv.ShortFileNameAt(*focused_slot);
        is_file = (i.second == LDV::EntryType::FILE);
        result = i.first;
    }

    if (isFile) {
        *isFile = is_file;
    }

    return result;
}

const char *window_file_list_t::TopItemSFN() {
    return ldv.ShortFileNameAt(0).first;
}

window_file_list_t::window_file_list_t(window_t *parent, Rect16 rc)
    : AddSuperWindow(parent, rc)
    , focused_item_delegate(string_view_utf8(), nullptr) {

    assert(max_items_on_screen_count() <= max_max_items_on_screen);

    DisableLongHoldScreenAction();
    Enable();
    strlcpy(sfn_path, "/usb", FILE_PATH_BUFFER_LEN);
}

void window_file_list_t::unconditionalDraw() {
    // Only paint background (in uncoditionalDraw) if it is really needed, filling the rect takes time...
    if (should_paint_background) {
        super::unconditionalDraw();
    }

    const auto focused_slot = this->focused_slot();

    for (int i = 0, end = current_items_on_screen_count(); i < end; i++) {
        if (valid_slots[i]) {
            continue;
        }

        valid_slots.set(i);

        const auto item_rect = slot_rect(i);

        if constexpr (GuiDefaults::MenuLinesBetweenItems) {
            if (flags.invalid_background && i < end - 1) {
                display::DrawLine(point_ui16(Left() + GuiDefaults::MenuItemDelimiterPadding.left, item_rect.Top() + item_rect.Height()),
                    point_ui16(Left() + Width() - GuiDefaults::MenuItemDelimiterPadding.right, item_rect.Top() + item_rect.Height()), COLOR_DARK_GRAY);
            }
        }

        // Return item; return_item_delegate handles both focused and unfocused return item
        if (is_return_slot(i)) {
            return_item_delegate.Print(item_rect);
            continue;
        }

        // Focused item, scrolling text
        else if (IsFocused() && focused_slot == i) {
            focused_item_delegate.Print(item_rect);
            continue;
        }

        // General list item
        else {
            FL_LABEL label(itemText(i), itemIcon(i));
            label.Print(item_rect);
        }
    }

    // no need fill the rest of the window with background, since there is no rest of the window
}

void window_file_list_t::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::TEXT_ROLL:
        if (IsFocused() && focused_item_delegate.is_focused()) {
            focused_item_delegate.Roll();
            if (focused_item_delegate.IsInvalid()) {
                invalidate_slot(focused_slot());
            }
        }
        break;

    default:
        break;
    }

    SuperWindowEvent(sender, event, param);
}

void window_file_list_t::SetRoot(char *rootPath) {
    root = rootPath;
}

void window_file_list_t::invalidate(Rect16 validation_rect) {
    valid_slots.reset();
    return_item_delegate.Invalidate(); // Definitely needs to be redrawn (actually necessary)
    focused_item_delegate.Invalidate();
    should_paint_background = true;
    super::invalidate(validation_rect);
}

void window_file_list_t::invalidate_slot(std::optional<int> slot) {
    if (!slot) {
        return;
    }

    valid_slots.reset(*slot);
    super::invalidate(slot_rect(*slot));
    gui_invalidate();
}

void window_file_list_t::invalidate_all_slots() {
    valid_slots.reset();
    return_item_delegate.Invalidate(); // Definitely needs to be redrawn (actually necessary)
    focused_item_delegate.Invalidate();
    super::invalidate(GetRect());
    gui_invalidate();
}

void window_file_list_t::validate(Rect16 validation_rect) {
    super::validate(validation_rect);
    should_paint_background = false;
}

const img::Resource *window_file_list_t::itemIcon(int slot) const {
    auto item = ldv.LongFileNameAt(slot);
    const bool isFile = item.second == LDV::EntryType::FILE;
    if (!item.first) {
        // this should normally not happen, visible_count shall limit indices to valid items only
        return nullptr; // ... but getting ready for the unexpected
    }
    return isFile ? nullptr : &img::folder_full_16x16;
}

// special handling for the link back to printing screen - i.e. ".." will be renamed to "Home"
// and will get a nice house-like icon
string_view_utf8 window_file_list_t::itemText(int slot) const {
    string_view_utf8 itemText;
    auto item = ldv.LongFileNameAt(slot);

    // this MakeRAM is safe - render_text (below) finishes its work and the local string item.first is then no longer needed
    return string_view_utf8::MakeRAM((const uint8_t *)item.first);
}

bool window_file_list_t::is_return_slot(const int slot) const {
    const auto item = ldv.LongFileNameAt(slot);
    return slot == 0 && IsPathRoot(sfn_path) && strcmp(item.first, "..") == 0;
}
