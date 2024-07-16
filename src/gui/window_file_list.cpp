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

    // -1 because ldv counts ".." as index -1 :/
    [[maybe_unused]] const auto new_window_offset = ldv.set_window_offset(set - 1);
    assert(new_window_offset == set - 1);

    // This must be called AFTER ldv.set_window_offset, as it updates the texts based on the newly shifted window
    WindowMenuVirtualBase::set_scroll_offset(set);
}

bool window_file_list_t::IsPathRoot(const char *path) {
    return (path[0] == 0 || (root && strcmp(path, root) == 0));
}

void window_file_list_t::Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN) {
    ldv.ChangeDirectory(
        sfn_path,
        (sort == WF_SORT_BY_NAME) ? LDV::SortPolicy::BY_NAME : LDV::SortPolicy::BY_CRMOD_DATETIME,
        topSFN ?: sfnAtCursor);

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

    WindowMenuVirtual::setup_items();

    // Focus item if appropriate
    {
        std::optional<int> target_focused_index;

        // If we've been given sfnAtCursor, try looking it up in the ldv window
        if (sfnAtCursor && sfnAtCursor[0]) {
            for (int i = 0, e = ldv.WindowSize(); i < e; i++) {
                if (!strcmp(sfnAtCursor, ldv.ShortFileNameAt(i).first)) {
                    target_focused_index = scroll_offset() + i;
                    break;
                }
            }
        }

        if (!target_focused_index && should_focus_item_on_init()) {
            // Try avoid highlighting ".." if there's any file in the dir
            target_focused_index = (item_count() > 1) ? 1 : 0;
        }

        move_focus_to_index(target_focused_index);
    }
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
    : WindowMenuVirtual(parent, rc, CloseScreenReturnBehavior::no) {

    assert(max_items_on_screen_count() <= max_max_items_on_screen);

    DisableLongHoldScreenAction();
    Enable();
    strlcpy(sfn_path, "/usb", FILE_PATH_BUFFER_LEN);
}

void window_file_list_t::SetRoot(char *rootPath) {
    root = rootPath;
}

void window_file_list_t::setup_item(ItemVariant &variant, int index) {
    assert(index_to_slot(index));

    const auto &entry = ldv.LongFileNameAt(*index_to_slot(index));

    if (index == 0 && IsPathRoot(sfn_path) && strcmp(entry.first, "..") == 0) {
        variant.emplace<MI_RETURN>();

    } else {
        // This is ok - file list keeps the address of the item same & valid while the item is in the window
        const auto label = string_view_utf8::MakeRAM(entry.first);
        const auto icon = (entry.second == LDV::EntryType::DIR) ? &img::folder_full_16x16 : nullptr;
        variant.emplace<WindowMenuItem>(label, icon);
    }
}
