/**
 * @file window_menu_bar.hpp
 * @brief menu scrollbar
 */

#pragma once

#include "window_menu.hpp"

class MenuScrollbar : public AddSuperWindow<window_t> {
    WindowMenu &menu;
    uint8_t max_items_on_screen = 0;
    uint8_t total_items = 0;
    uint8_t index_of_first = 0;

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    MenuScrollbar(window_t *parent, Rect16 rect, WindowMenu &menu);
};
