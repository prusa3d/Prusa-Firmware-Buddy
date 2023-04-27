/*
 * window_file_list.hpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include <stdbool.h>
#include "filename_defs.h"
#include "window.hpp"
#include "display_helper.h"
#include "lazyfilelist.h"
#include "text_roll.hpp"
#include "WindowMenuItems.hpp"
#include "GuiDefaults.hpp"
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

class FL_LABEL : public WI_LABEL_t {
public:
    FL_LABEL(string_view_utf8 label, const png::Resource *icon)
        : WI_LABEL_t(label, icon, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) {}
};

class window_file_list_t : public AddSuperWindow<window_aligned_t> {
public:
    static constexpr padding_ui8_t padding = GuiDefaults::MenuPaddingItems;
    static constexpr Rect16::Height_t font_h = 19; // fonts are not constexpr
    static constexpr Rect16::Height_t item_height = font_h + padding.top + padding.bottom;

    static constexpr size_t LazyDirViewSize = GuiDefaults::FileBrowserRect.Height() / item_height;
    using LDV = LazyDirView<LazyDirViewSize>;

protected:
    LDV ldv;
    static char *root; // this is a Short-File-Name path to the root of the dialog

    color_t color_text;
    font_t *font;

    int count; // total number of files/entries in a dir
    int index; // selected index - cursor position within the visible items

    std::array<bool, LazyDirViewSize> valid_items;
    bool entire_window_invalid;

public:
    //TODO private
    char sfn_path[FILE_PATH_BUFFER_LEN]; // this is a Short-File-Name path where we start the file dialog
public:
    window_file_list_t(window_t *parent, Rect16 rc); // height is calculated from LazyDirViewSize
    void Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN);

    void SetItemIndex(int index);
    const char *TopItemSFN();
    const char *CurrentLFN(bool *isFile = nullptr) const;
    const char *CurrentSFN(bool *isFile = nullptr) const;
    static void SetRoot(char *rootPath);

    /// @return true if path is either empty or contains just a "/"
    static bool IsPathRoot(const char *path);

    void RollUp();
    void RollDown();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual void invalidate(Rect16 validation_rect) override;
    virtual void validate(Rect16 validation_rect) override;

    void invalidateItem(int index); // invalidate one item in list
    virtual void unconditionalDraw() override;
    void inc(int dif);                ///< negative values move cursor in opposite direction
    void roll_screen(int dif);        ///< negative values move cursor in opposite direction
    void selectNewItem();             // sets focus and activates text rolling
    Rect16 itemRect(int index) const; // get rectangle of item with target index
    string_view_utf8 itemText(int index) const;
    const png::Resource *itemIcon(int index) const;
    FL_LABEL activeItem;   ///< used for text rolling
    MI_RETURN return_item; ///< used for return item

private:
    [[nodiscard]] bool is_return_item(const int index) const;
};
