/*
 * window_file_list.hpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include <stdbool.h>
#include <bitset>

#include <buddy/filename_defs.hpp>
#include "window.hpp"
#include "display_helper.h"
#include "lazyfilelist.hpp"
#include "text_roll.hpp"
#include <i_window_menu_item.hpp>
#include <window_menu_virtual.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <array>

// This enum value is stored to eeprom as file sort settings
typedef enum {
    WF_SORT_BY_TIME,
    WF_SORT_BY_NAME
} WF_Sort_t;

class GuiFileSort {
    WF_Sort_t sort;

    GuiFileSort();
    GuiFileSort(const GuiFileSort &) = delete;
    static GuiFileSort &instance();

public:
    static WF_Sort_t Get();
    static void Set(WF_Sort_t val);
};

class FL_LABEL final : public IWindowMenuItem {
public:
    FL_LABEL(const string_view_utf8 &label, const img::Resource *icon)
        : IWindowMenuItem(label, icon, is_enabled_t::yes, is_hidden_t::no) {}
};

// Items sized for WindowMenuItem. The other item being used is MI_RETURN,
// which adds no members over IWindowMenuItem.
class window_file_list_t : public WindowMenuVirtualSized<WindowMenuVirtualBase::default_item_buffer_size, sizeof(WindowMenuItem)> {

public:
    static constexpr const char *root = "/usb";
    using LDV = LazyDirView<item_buffer_size>;

public:
    inline int item_count() const final {
        return ldv.TotalFilesCount();
    }

    void set_scroll_offset(int set) final;

public:
    // TODO private
    char sfn_path[filename_defs::path_buffer_size]; // this is a Short-File-Name path where we start the file dialog

public:
    window_file_list_t(window_t *parent, Rect16 rc); // height is calculated from LazyDirViewSize
    void Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN);

    const char *TopItemSFN();
    const char *CurrentLFN(bool *isFile = nullptr) const;
    const char *CurrentSFN(bool *isFile = nullptr) const;

    /// @return true if path is either empty or equal to the root path
    static bool IsPathRoot(const char *path);

protected:
    void setup_item(ItemVariant &variant, int index) final;

protected:
    LDV ldv;
};
