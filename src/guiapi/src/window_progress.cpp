// window_progress.cpp
#include "window_progress.hpp"
#include "gui.hpp"
#include <algorithm>

static const constexpr uint8_t WINDOW_PROGRESS_MAX_TEXT = 16;

/*****************************************************************************/
//window_numberless_progress_t
window_numberless_progress_t::window_numberless_progress_t(window_t *parent, Rect16 rect, color_t cl_progress, color_t cl_back)
    : AddSuperWindow<window_t>(parent, rect)
    , color_progress(cl_progress) {
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
    if (rc.Width())
        display::FillRect(rc, GetBackColor());
    rc = Left();
    rc = Rect16::Width_t(progress_w);
    if (rc.Width())
        display::FillRect(rc, color_progress);
}

/*****************************************************************************/
//window_progress_t
void window_progress_t::SetValue(float val) {
    const float value = std::max(min, std::min(val, max));
    numb.SetValue(value);
    progr.SetProgressPercent(value);
}

window_progress_t::window_progress_t(window_t *parent, Rect16 rect, uint16_t h_progr, color_t cl_progress, color_t cl_back)
    : AddSuperWindow<window_frame_t>(parent, rect)
    , progr(this, { rect.Left(), rect.Top(), rect.Width(), h_progr }, cl_progress, cl_back)
    , numb(this, { rect.Left(), int16_t(rect.Top() + h_progr), rect.Width(), uint16_t(rect.Height() - h_progr) })
    , min(0)
    , max(100) {
    Disable();
    numb.format = "%.0f%%";
    numb.SetAlignment(Align_t::Center());
}

void window_progress_t::SetFont(font_t *val) {
    numb.SetFont(val);
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
