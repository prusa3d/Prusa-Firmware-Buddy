#include "window_progress.hpp"

#include "display.hpp"
#include <algorithm>
#include <bsod/bsod.h>

WindowProgressBarBase::WindowProgressBarBase(window_t *parent, Rect16 rect, Color fg_color, Color bg_color)
    : window_t { parent, rect }
    , fg_color { fg_color }
    , progress_in_pixels { 0 } {
    SetBackColor(bg_color);
}

void WindowProgressBarBase::set_progress_percent(float val) {
    const float min = 0;
    const float max = 100;
    const float value = std::max(min, std::min(val, max));
    uint16_t px = static_cast<uint16_t>((value * Width()) / max);
    if (px != progress_in_pixels) {
        progress_in_pixels = px;
        Invalidate();
    }
}

static Rect16 bg_rect(const Rect16 &rect, uint16_t progress_in_pixels) {
    return Rect16(
        rect.Left() + progress_in_pixels,
        rect.Top(),
        rect.Width() - progress_in_pixels,
        rect.Height());
}

static Rect16 fg_rect(const Rect16 &rect, uint16_t progress_in_pixels) {
    return Rect16(
        rect.Left(),
        rect.Top(),
        progress_in_pixels,
        rect.Height());
}

void WindowProgressBar::unconditionalDraw() {
    const Rect16 rect = GetRect();
    const Color bg_color = GetBackColor();
    display::fill_rect(bg_rect(rect, progress_in_pixels), bg_color);
    display::fill_rect(fg_rect(rect, progress_in_pixels), fg_color);
}

void WindowRoundedProgressBar::unconditionalDraw() {
    const Rect16 rect = GetRect();
    const Color bg_color = GetBackColor();
    const Color screen_background = GetParent() ? GetParent()->GetBackColor() : bg_color;
    display::draw_rounded_rect(
        bg_rect(rect, progress_in_pixels),
        screen_background,
        bg_color,
        corner_radius,
        progress_in_pixels ? MIC_TOP_RIGHT | MIC_BOT_RIGHT : MIC_ALL_CORNERS);
    display::draw_rounded_rect(
        fg_rect(rect, progress_in_pixels),
        screen_background,
        fg_color,
        corner_radius,
        MIC_ALL_CORNERS | MIC_ALT_CL_TOP_RIGHT | MIC_ALT_CL_BOT_RIGHT,
        progress_in_pixels == Width() ? screen_background : bg_color);
}

WindowProgressCircles::WindowProgressCircles(window_t *parent, Rect16 rect, uint8_t max_circles_)
    : window_t(parent, rect)
    , max_circles(max_circles_) {
    debug_assert(max_circles > 0);
    debug_assert(rect.Width() >= (rect.Height() - 1) * max_circles);
}

void WindowProgressCircles::unconditionalDraw() {
    debug_assert(!flags.has_round_corners); // Not implemented

    window_t::unconditionalDraw(); // draws background

    const auto &drawn_rect { GetRect() };
    const auto delimiter = (drawn_rect.Width() - max_circles * (drawn_rect.Height())) / (max_circles);
    int16_t current_x { static_cast<int16_t>(drawn_rect.Left() + delimiter / 2) };

    for (size_t i = 0; i < max_circles; ++i) {
        Rect16 circle_to_draw {
            current_x,
            drawn_rect.Top(),
            static_cast<Rect16::Width_t>(drawn_rect.Height()),
            static_cast<Rect16::Height_t>(drawn_rect.Height()),
        };
        const Color color = i == current_index || (!one_circle_mode && i < current_index) ? color_on : color_off;

        const auto corner_radius =
            [&]() {
                if (circle_to_draw.Height() <= 8) {
                    return circle_to_draw.Height() * 80 / 100;
                } else if (circle_to_draw.Height() < 15) {
                    return circle_to_draw.Height() * 70 / 100;
                } else if (circle_to_draw.Height() < 25) {
                    return circle_to_draw.Height() * 60 / 100;
                } else {
                    return circle_to_draw.Height() * 52 / 100;
                }
            }();

        // We don't have a simple way of drawing circle on the screen, but drawing rounded rectangle with the magic constant (found experimentally) produces 'good enough' circles
        display::draw_rounded_rect(circle_to_draw, GetBackColor(), color, corner_radius, MIC_ALL_CORNERS);

        current_x += drawn_rect.Height() + delimiter;
    }
}

void WindowProgressCircles::set_index(uint8_t new_index) {
    if (current_index == new_index) {
        return;
    }
    current_index = new_index;
    Invalidate();
}

void WindowProgressCircles::set_on_color(Color clr) {
    color_on = clr;
    Invalidate();
}

void WindowProgressCircles::set_off_color(Color clr) {
    color_off = clr;
    Invalidate();
}

void WindowProgressCircles::set_one_circle_mode(bool new_mode) {
    one_circle_mode = new_mode;
    Invalidate();
}

void WindowProgressCircles::set_max_circles(uint8_t new_max_circles) {
    debug_assert(new_max_circles > 0);
    max_circles = new_max_circles;
    Invalidate();
}
