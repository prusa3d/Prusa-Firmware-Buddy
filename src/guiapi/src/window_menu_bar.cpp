/**
 * @file window_menu_bar.cpp
 */

#include "window_menu_bar.hpp"

MenuScrollbar::MenuScrollbar(window_t *parent, Rect16 rect, IWindowMenu &menu)
    : AddSuperWindow<window_t>(parent, rect)
    , menu(menu) {}

void MenuScrollbar::unconditionalDraw() {
    if (state.item_count == 0) { // scrollbar should be hidden, check it anyway to prevent divide by 0
        return;
    }

    const Rect16 available_rect = GetRect();
    const unsigned bar_height = unsigned(float(state.max_items_on_screen) / float(state.item_count) * float(available_rect.Height()));
    const unsigned bar_offset = float(state.scroll_offset) / float(state.item_count - state.max_items_on_screen) * float(available_rect.Height() - bar_height);

    const auto back_color = GetBackColor();

    // Draw background above bar
    if (bar_offset > 0) {
        display::FillRect(Rect16(available_rect.Left(), available_rect.Top(), available_rect.Width(), bar_offset), back_color);
    }

    // Draw bar
    display::FillRect(Rect16(available_rect.Left(), available_rect.Top() + bar_offset, available_rect.Width(), bar_height), COLOR_SILVER);

    // Draw background below bar
    if (const auto y_start = bar_offset + bar_height; y_start < available_rect.Height()) {
        display::FillRect(Rect16(available_rect.Left(), available_rect.Top() + y_start, available_rect.Width(), available_rect.Height() - y_start), back_color);
    }
}

void MenuScrollbar::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {

    case GUI_event_t::LOOP: {
        const State new_state {
            .max_items_on_screen = menu.max_items_on_screen_count(),
            .item_count = menu.item_count(),
            .scroll_offset = menu.scroll_offset(),
        };
        if (state != new_state) {
            state = new_state;
            Invalidate();
        }

        set_visible(state.item_count > state.max_items_on_screen);
        break;
    }

    default:
        // discard other events
        break;
    }
}
