/*
 * window_file_list.hpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include <stdbool.h>
#include <bitset>

#include "filename_defs.h"
#include "window.hpp"
#include "display_helper.h"
#include "lazyfilelist.hpp"
#include "text_roll.hpp"
#include "WindowMenuItems.hpp"
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

class FL_LABEL : public IWindowMenuItem {
public:
    FL_LABEL(string_view_utf8 label, const img::Resource *icon)
        : IWindowMenuItem(label, icon, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) {}
};

class window_file_list_t : public AddSuperWindow<IWindowMenu> {

public:
    static constexpr int max_max_items_on_screen = GuiDefaults::FileBrowserRect.Height() / item_height();
    using LDV = LazyDirView<max_max_items_on_screen>;

public:
    inline int item_count() const final {
        return item_count_;
    }

public: // Scroll stuff
    void set_scroll_offset(int set) final;

public: // Focus stuff
    std::optional<int> focused_item_index() const final;

    bool move_focus_to_index(std::optional<int> index) final;

    [[nodiscard]] bool is_return_slot(const int slot) const;

public:
    // TODO private
    char sfn_path[FILE_PATH_BUFFER_LEN]; // this is a Short-File-Name path where we start the file dialog
public:
    window_file_list_t(window_t *parent, Rect16 rc); // height is calculated from LazyDirViewSize
    void Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN);

    const char *TopItemSFN();
    const char *CurrentLFN(bool *isFile = nullptr) const;
    const char *CurrentSFN(bool *isFile = nullptr) const;
    static void SetRoot(char *rootPath);

    /// @return true if path is either empty or contains just a "/"
    static bool IsPathRoot(const char *path);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    virtual void invalidate(Rect16 validation_rect) override;
    void invalidate_slot(std::optional<int> slot);
    void invalidate_all_slots();

    virtual void validate(Rect16 validation_rect) override;

    virtual void unconditionalDraw() override;
    string_view_utf8 itemText(int slot) const;
    const img::Resource *itemIcon(int slot) const;

protected:
    LDV ldv;
    static char *root; // this is a Short-File-Name path to the root of the dialog

    color_t color_text = GuiDefaults::ColorText;

    int item_count_; ///< total number of files/entries in a dir
    std::optional<int> focused_index_; // selected index - cursor position within the visible items

    std::bitset<max_max_items_on_screen> valid_slots;

    /// We usually want to avoid painting background, because it fills the whole menu rect, which is very inefficient.
    bool should_paint_background = true;

protected:
    FL_LABEL focused_item_delegate; ///< used for text rolling
    MI_RETURN return_item_delegate; ///< used for return item
};
