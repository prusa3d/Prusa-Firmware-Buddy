#include <window_colored_rect.hpp>

#include "display.hpp"

window_colored_rect::window_colored_rect(window_t *parent, Rect16 rect)
    : window_t(parent, rect) {
}

void window_colored_rect::set_border_thickness(uint8_t new_thickness) {
    border_thickness = new_thickness;
    Invalidate();
}

void window_colored_rect::set_parent_color(Color new_parent_color) {
    parent_color = new_parent_color;
    Invalidate();
}

void window_colored_rect::unconditionalDraw() {
    static constexpr uint8_t threshold_adjustment { 64 }; // used so that for example only really black colours will have a white background instead of all that are beyond halfway point

    // roughly convert the color to grayscale and check whether it's closer to black or white
    auto lum = GetBackColor().to_grayscale();

    bool is_closer_to_white = [&]() {
        if (parent_color == COLOR_BLACK) {
            return lum > 127 - threshold_adjustment; // only colours really close to black will have white border
        } else if (parent_color == COLOR_WHITE) {
            return lum > 127 + threshold_adjustment; // only colours really close to white will have black border
        } else {
            return lum > 127;
        }
    }();

    Rect16 rect = GetRect();

    // draw the border rect
    if (flags.has_round_corners) {
        display::draw_rounded_rect(rect, parent_color, is_closer_to_white ? COLOR_BLACK : COLOR_WHITE, GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS);
    } else {
        display::draw_rect(rect, is_closer_to_white ? COLOR_BLACK : COLOR_WHITE);
    }

    // shrink the rect for the actual color
    rect = static_cast<Rect16::Top_t>(rect.Top() + border_thickness);
    rect = static_cast<Rect16::Left_t>(rect.Left() + border_thickness);
    rect = static_cast<Rect16::Width_t>(rect.Width() - border_thickness * 2);
    rect = static_cast<Rect16::Height_t>(rect.Height() - border_thickness * 2);

    // draw the actual rect
    if (flags.has_round_corners) {
        display::draw_rounded_rect(rect,
            is_closer_to_white ? COLOR_BLACK : COLOR_WHITE, GetBackColor(), GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS);
    } else {
        display::fill_rect(rect, GetBackColor());
    }
}
