// window_list.hpp

#pragma once

#include "window.hpp"

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
    void SetItemIndex(int idx);
    void SetCallback(window_list_item_t *fnc);
    void SetTopIndex(int idx);

    int GetItemCount() const { return count; }
    int GetItemIndex() const { return index; }
    int GetTopIndex() const { return top_index; }

    window_list_t(window_t *parent, rect_ui16_t rect);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};
