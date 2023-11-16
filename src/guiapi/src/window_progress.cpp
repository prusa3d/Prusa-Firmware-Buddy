// window_progress.cpp
#include "window_progress.hpp"
#include "gui.hpp"
#include <algorithm>

static const constexpr uint8_t WINDOW_PROGRESS_MAX_TEXT = 16;

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

/*****************************************************************************/
// window_progress_t

static constexpr int PROGRESS_NUM_Y_OFFSET = 10;

void window_progress_t::SetValue(float val) {
    const float value = std::max(min, std::min(val, max));
    numb.SetValue(value);
    progr.SetProgressPercent(value);
}

window_progress_t::window_progress_t(window_t *parent, Rect16 rect, uint16_t h_progr, color_t cl_progress, color_t cl_back, int corner_radius)
    : AddSuperWindow<window_frame_t>(parent, rect)
    , progr(this, { rect.Left(), rect.Top(), rect.Width(), h_progr }, cl_progress, cl_back, corner_radius)
    , numb(this, { rect.Left(), int16_t(rect.Top() + h_progr + PROGRESS_NUM_Y_OFFSET), rect.Width(), uint16_t(rect.Height() - h_progr - PROGRESS_NUM_Y_OFFSET) })
    , min(0)
    , max(100) {
    Disable();
    numb.format = "%.0f%%";
    numb.SetAlignment(Align_t::Center());
}

void window_progress_t::set_font(font_t *val) {
    numb.set_font(val);
}

void window_progress_t::SetProgressColor(color_t clr) {
    progr.SetColor(clr);
}

void window_progress_t::SetNumbColor(color_t clr) {
    numb.SetTextColor(clr);
}

void window_progress_t::SetProgressHeight(Rect16::Height_t height) {
    if (progr.Height() != height) {
        progr.Resize(height);
        progr.Invalidate();
        numb -= height;
        numb.Invalidate();
    }
}

void window_progress_t::SetProgressPercent(uint8_t val) {
    if (val != numb.GetValue()) {
        progr.SetProgressPercent(val);
        numb.SetValue(val);
        Invalidate();
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
