#pragma once

#include "window_menu.h"
#include "IWinMenuContainer.hpp"
#include <stdint.h>

#pragma pack(push, 1)

struct window_menu_t {
    window_t win;
    color_t color_back;
    color_t color_text;
    color_t color_disabled;
    font_t *font;
    padding_ui8_t padding;
    rect_ui16_t icon_rect;
    uint8_t alignment;
    int count;
    int index;
    int top_index;
    bool selected;
    IWinMenuContainer *pContainer;
    //uint8_t mode;
    void *data;
    uint8_t src_event; // source event
    void *src_param;   // source event data
};

#define CAST_WI_SPIN_FL(MI) (reinterpret_cast<WI_SPIN_FL_t &>(psmd->items[MI].item.Get()))
#define CAST_WI_SPIN(MI)    (reinterpret_cast<WI_SPIN_t &>(psmd->items[MI].item.Get()))

typedef enum {
    WI_LABEL = 0, // types - exclusive
    WI_SPIN,
    WI_SWITCH,
    WI_SELECT,
    WI_SPIN_FL,

    // WI_DISABLED = 1 << 8, // flags - non exclusive
    // WI_HIDDEN = 1 << 9,
} window_item_type_t;

#pragma pack(pop)
