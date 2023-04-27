/**
 * @file window_menu_bar.cpp
 */

#include "window_menu_bar.hpp"

MenuScrollbar::MenuScrollbar(window_t *parent, Rect16 rect, WindowMenu &menu)
    : AddSuperWindow<window_t>(parent, rect)
    , menu(menu) {}

void MenuScrollbar::unconditionalDraw() {
    if (total_items == 0) // scrollbar should be hidden, check it anyway to prevent divide by 0
        return;

    uint16_t scroll_item_height = Height() / unsigned(total_items);
    Rect16::Height_t bar_height = Rect16::Height_t(max_items_on_screen * scroll_item_height);

    if (total_items - index_of_first > max_items_on_screen) {
        // normal draw
        uint16_t bar_y = Top() + unsigned(index_of_first) * unsigned(scroll_item_height);

        Rect16 rc = GetRect();
        rc = Rect16::Height_t(bar_y - rc.Top());
        display::FillRect(rc, GetBackColor());
        rc = Rect16::Top_t(bar_y);
        rc = bar_height;
        display::FillRect(rc, COLOR_SILVER);
        rc += Rect16::Top_t(rc.Height());
        rc = GetRect().Intersection(rc); // cut the height
        display::FillRect(rc, GetBackColor());
    } else {
        // draw at the last position
        // there would be normally gap due rounding error
        Rect16 rc = GetRect();
        rc -= bar_height;
        display::FillRect(rc, GetBackColor());
        rc += Rect16::Top_t(rc.Height());
        rc = bar_height;
        display::FillRect(rc, COLOR_SILVER);
    }
}

void MenuScrollbar::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {
    case GUI_event_t::LOOP:
        if (max_items_on_screen != menu.GetMaxItemsOnScreen()) {
            max_items_on_screen = menu.GetMaxItemsOnScreen();
            Invalidate();
        }
        if (total_items != menu.GetCount()) {
            total_items = menu.GetCount();
            Invalidate();
        }
        if (index_of_first != menu.GetIndexOfFirst()) {
            index_of_first = menu.GetIndexOfFirst();
            Invalidate();
        }

        // Hide it if it cannot be drawn
        if (max_items_on_screen == 0 || total_items == 0 || total_items <= max_items_on_screen) {
            Hide();
        } else {
            Show();
        }
        break;
    default:
        // discard other events
        break;
    }
}
