/**
 * @file window_menu_bar.hpp
 * @brief menu scrollbar
 */

#pragma once

#include "i_window_menu.hpp"

class MenuScrollbar : public AddSuperWindow<window_t> {

public:
    MenuScrollbar(window_t *parent, Rect16 rect, IWindowMenu &menu);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    IWindowMenu &menu;

private:
    struct State {

    public:
        int max_items_on_screen = 0;
        int item_count = 0;
        int scroll_offset = 0;

    public:
        bool operator==(const State &) const = default;
        bool operator!=(const State &) const = default;
    };
    State state;
};
