/*
 * window_file_list.hpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include <stdbool.h>

#include "window.hpp"
#include "ff.h"
#include "file_list_defs.h"
#include "display_helper.h"
#include "../common/marlin_vars.h" // for FILE_PATH_MAX_LEN
#include "lazyfilelist.h"
#include "text_roll.hpp"
#include "WindowMenuItems.hpp"

static constexpr size_t LazyDirViewSize = 9; //cannot be calculated, font is not constexpr
using LDV9 = LazyDirView<LazyDirViewSize>;

// This enum value is stored to eeprom as file sort settings
typedef enum {
    WF_SORT_BY_TIME,
    WF_SORT_BY_NAME

} WF_Sort_t;

extern WF_Sort_t screen_filebrowser_sort;

inline LDV9 *LDV_Get(void) {
    static LDV9 ldv;
    return &ldv;
}

class FL_LABEL : public WI_LABEL_t {
public:
    FL_LABEL(string_view_utf8 label, uint16_t id_icon)
        : WI_LABEL_t(label, id_icon, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click(IWindowMenu &window_menu) {}
};

struct window_file_list_t : public window_aligned_t {
    color_t color_text;
    font_t *font;
    padding_ui8_t padding;
    txtroll_t roll;
    int count;                        // total number of files/entries in a dir
    int index;                        // selected index - cursor position within the visible items
    LDV9 *ldv;                        // I'm a C-pig and I need a pointer to my LazyDirView class instance ... subject to change when this gets rewritten to C++
    char sfn_path[FILE_PATH_MAX_LEN]; // this is a Short-File-Name path where we start the file dialog
    window_file_list_t(window_t *parent, Rect16 rect);
    void Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN);

public:
    void SetItemIndex(int index);
    const char *TopItemSFN();
    const char *CurrentLFN(bool *isFile);
    const char *CurrentSFN(bool *isFile);

    /// @return true if path is either empty or contains just a "/"
    static bool IsPathRoot(const char *path);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    virtual void unconditionalDraw() override;
    void inc(int dif);   ///< negative values move cursor in opposite direction
    FL_LABEL activeItem; ///< used for text rolling
};
