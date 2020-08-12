/*
 * window_file_list.hpp
 *
 *  Created on: 23. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include "window.hpp"
#include "ff.h"
#include <stdbool.h>
#include "file_list_defs.h"
#include "display_helper.h"
#include "../common/marlin_vars.h" // for FILE_PATH_MAX_LEN
#include "lazyfilelist.h"
using LDV9 = LazyDirView<9>;

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

struct window_file_list_t : public window_t {
    color_t color_text;
    font_t *font;
    padding_ui8_t padding;
    txtroll_t roll;
    int count;                        // total number of files/entries in a dir
    int index;                        // selected index - cursor position within the visible items
    LDV9 *ldv;                        // I'm a C-pig and I need a pointer to my LazyDirView class instance ... subject to change when this gets rewritten to C++
    char sfn_path[FILE_PATH_MAX_LEN]; // this is a Short-File-Name path where we start the file dialog
    uint8_t alignment;
    window_file_list_t(window_t *parent, Rect16 rect);
    void Load(WF_Sort_t sort, const char *sfnAtCursor, const char *topSFN);

public:
    void SetItemIndex(int index);
    const char *TopItemSFN();
    const char *CurrentLFN(bool *isFile);
    const char *CurrentSFN(bool *isFile);

    /// @return true if path is either empty or contains just a "/"
    static bool IsPathRoot(const char *path);

private:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    void inc(int dif);
    void dec(int dif);
    void init_text_roll();
};
