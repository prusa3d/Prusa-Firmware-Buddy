/**
 * @file scroll_bar.cpp
 * @author Radek Vana
 * @date 2022-01-21
 */

#include "scroll_bar.hpp"

ScrollBar::ScrollBar(window_t *parrent, Rect16::Width_t w)
    : AddSuperWindow<window_t>(parrent, calculateRect(parrent->GetRect(), w)) {
}

Rect16 ScrollBar::calculateRect(Rect16 parrent_rect, Rect16::Width_t w) {
    parrent_rect = Rect16::Left_t(parrent_rect.Left() + parrent_rect.Width() - w);
    parrent_rect = w;
    return parrent_rect;
}

void ScrollBar::SetHeightToScroll(Rect16::Height_t height) {
    scroll_height = height;
    offset = 0;
    Invalidate();
}

void ScrollBar::SetScrollOffset(Rect16::Height_t offset_) {
    offset_ = (offset_ >= scroll_height) ? scroll_height : offset_;
    if (offset == offset_) {
        return; // do not invalidate
    }
    offset = offset_;
    Invalidate();
}

void ScrollBar::unconditionalDraw() {
    super::unconditionalDraw(); // draw background
    Rect16 rc = GetRect();
    Rect16::Height_t h = rc.Height();
    if (h >= scroll_height) {
        return;
    }

    float h_fl = float(h);

    float bar_size = h_fl * h_fl / float(scroll_height);
    float offset_scaled = float(offset) * (h_fl - bar_size) / float(scroll_height);

    rc = Rect16::Height_t(bar_size);
    rc += Rect16::Top_t(offset_scaled);

    display::DrawRect(rc, COLOR_SILVER);
}
