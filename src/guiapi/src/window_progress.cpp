// window_progress.cpp
#include "window_progress.hpp"
#include "gui.hpp"
#include <algorithm>

/*****************************************************************************/
// window_numberless_progress_t
window_numberless_progress_t::window_numberless_progress_t(window_t *parent, Rect16 rect, color_t cl_progress, color_t cl_back, int corner_radius)
    : AddSuperWindow<window_t>(parent, rect)
    , color_progress(cl_progress)
    , corner_radius(corner_radius) {
    SetProgressInPixels(0);
    SetBackColor(cl_back);
}

void window_numberless_progress_t::SetProgressInPixels(uint16_t px) {
    if (px != progress_in_pixels) {
        progress_in_pixels = px;
        Invalidate();
    }
}

void window_numberless_progress_t::SetProgressPercent(float val) {
    const float min = 0;
    const float max = 100;
    const float value = std::max(min, std::min(val, max));
    SetProgressInPixels((value * Width()) / max);
}

uint16_t window_numberless_progress_t::GetProgressPixels() const {
    return progress_in_pixels;
}

void window_numberless_progress_t::SetColor(color_t clr) {
    if (clr != color_progress) {
        color_progress = clr;
        Invalidate();
    }
}

void window_numberless_progress_t::unconditionalDraw() {
    Rect16 rc = GetRect();
    const uint16_t progress_w = std::min(GetProgressPixels(), uint16_t(rc.Width()));
    rc += Rect16::Left_t(progress_w);
    rc -= Rect16::Width_t(progress_w);

    color_t screen_background = GetParent() ? GetParent()->GetBackColor() : GetBackColor();

    // Draw background
    if (rc.Width()) {
        if (corner_radius) {
            uint8_t corner_flag = Left() == rc.Left() ? MIC_ALL_CORNERS : MIC_TOP_RIGHT | MIC_BOT_RIGHT;
            display::DrawRoundedRect(rc, screen_background, GetBackColor(), corner_radius, corner_flag);
        } else {
            display::FillRect(rc, GetBackColor());
        }
    }
    rc = Left();
    rc = Rect16::Width_t(progress_w);
    // Draw progress
    if (rc.Width()) {
        if (corner_radius) {
            color_t secondary_clr = GetProgressPixels() == GetRect().Width() ? screen_background : GetBackColor();
            display::DrawRoundedRect(rc, screen_background, color_progress, corner_radius,
                MIC_ALL_CORNERS | MIC_ALT_CL_TOP_RIGHT | MIC_ALT_CL_BOT_RIGHT, secondary_clr);
        } else {
            display::FillRect(rc, color_progress);
        }
    }
}

/*******************************************************************************/
// window_vertical_progress_t
window_vertical_progress_t::window_vertical_progress_t(window_t *parent, Rect16 rect, color_t cl_progress, color_t cl_back)
    : AddSuperWindow<window_t>(parent, rect)
    , color_progress(cl_progress) {
    SetBackColor(cl_back);
}

void window_vertical_progress_t::SetProgressColor(color_t clr) {
    if (clr != color_progress) {
        color_progress = clr;
        Invalidate();
    }
}

void window_vertical_progress_t::SetProgressWidth(uint16_t width) {
    if (width != Width()) {
        const Rect16::Width_t w(width);
        SetRect(Rect16(Left(), Top(), w, Height()));
        Invalidate();
    }
}

void window_vertical_progress_t::SetProgressInPixels(uint16_t px) {
    if (px != progress_in_pixels) {
        progress_in_pixels = px;
        Invalidate();
    }
}

void window_vertical_progress_t::SetProgressPercent(uint8_t val) {
    const uint8_t min = 0;
    const uint8_t max = 100;
    const uint8_t value = std::max(min, std::min(val, max));
    SetProgressInPixels(uint16_t((value * Height()) / max));
}

uint16_t window_vertical_progress_t::GetProgressPixels() const {
    return progress_in_pixels;
}

void window_vertical_progress_t::unconditionalDraw() {
    Rect16 rc = GetRect();
    const uint16_t progress_h = std::min(GetProgressPixels(), uint16_t(Height()));
    rc = Rect16::Height_t(Height() - progress_h);
    if (rc.Height()) {
        display::FillRect(rc, GetBackColor());
    }
    rc = Rect16::Top_t(Height() - progress_h);
    rc = Rect16::Height_t(progress_h);
    if (rc.Height()) {
        display::FillRect(rc, color_progress);
    }
}

WindowProgressCircles::WindowProgressCircles(window_t *parent, Rect16 rect, uint8_t max_circles_)
    : AddSuperWindow<window_t>(parent, rect)
    , max_circles(max_circles_) {
    assert(max_circles > 0);
    assert(rect.Width() >= (rect.Height() - 1) * max_circles);
}

void WindowProgressCircles::unconditionalDraw() {
    assert(!flags.has_round_corners); // Not implemented

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
        const color_t color = i == current_index || (!one_circle_mode && i < current_index) ? color_on : color_off;

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
        display::DrawRoundedRect(circle_to_draw, GetBackColor(), color, corner_radius, MIC_ALL_CORNERS);

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

void WindowProgressCircles::set_on_color(color_t clr) {
    color_on = clr;
    Invalidate();
}

void WindowProgressCircles::set_off_color(color_t clr) {
    color_off = clr;
    Invalidate();
}

void WindowProgressCircles::set_one_circle_mode(bool new_mode) {
    one_circle_mode = new_mode;
    Invalidate();
}
