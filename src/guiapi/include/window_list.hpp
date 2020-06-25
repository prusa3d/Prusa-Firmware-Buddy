// window_list.hpp

#pragma once

#include "window.hpp"

struct window_class_list_t {
    window_class_t cls;
};

struct window_list_t : window_t {
    color_t color_text;
    font_t *font;
    padding_ui8_t padding;
    uint8_t alignment;
    rect_ui16_t icon_rect;
    int count;
    int index;
    int top_index;
    window_list_item_t *list_item;

    void SetItemCount(int cnt);
    int GetItemCount() const { return count; }
};

extern const window_class_list_t window_class_list;
