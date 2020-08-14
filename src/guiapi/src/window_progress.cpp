// window_progress.c
#include "window_progress.hpp"
#include "gui.hpp"
#include <algorithm>

#define WINDOW_PROGRESS_MAX_TEXT 16

/*****************************************************************************/
//window_numberless_progress_t
window_numberless_progress_t::window_numberless_progress_t(window_t *parent, Rect16 rect, color_t cl_progress, color_t cl_back)
    : window_t(parent, rect)
    , color_progress(cl_progress)
    , progress_in_pixels(0) {
    color_back = cl_back;
}

void window_numberless_progress_t::SetProgress(uint16_t px) {
    if (px != progress_in_pixels) {
        progress_in_pixels = px;
        Invalidate();
    }
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
    Rect16 rc = rect;
    const uint16_t progress_w = std::min(progress_in_pixels, uint16_t(rc.Width()));
    rc += Rect16::Left_t(progress_w);
    rc -= Rect16::Width_t(progress_w);
    if (rc.Width())
        display::FillRect(rc, color_back);
    rc = rect.Left();
    rc = Rect16::Width_t(progress_w);
    if (rc.Width())
        display::FillRect(rc, color_progress);
}

/*****************************************************************************/
//window_progress_t
void window_progress_t::SetValue(float val) {
    const float value = std::max(min, std::min(val, max));
    numb.SetValue(value);
    progr.SetProgress((value * progr.rect.Width()) / max);
}

window_progress_t::window_progress_t(window_t *parent, Rect16 rect, uint16_t h_progr, color_t cl_progress, color_t cl_back)
    : window_frame_t(parent, rect)
    , progr(this, { rect.Left(), rect.Top(), rect.Width(), h_progr }, cl_progress, cl_back)
    , numb(this, { rect.Left(), int16_t(rect.Top() + h_progr), rect.Width(), uint16_t(rect.Height() - h_progr) })
    , min(0)
    , max(100) {
    Disable();
    numb.format = "%.0f%%";
    numb.alignment = ALIGN_CENTER;
}

void window_progress_t::SetFont(font_t *val) {
    numb.SetFont(val);
}

void window_progress_t::SetProgressColor(color_t clr) {
    progr.SetColor(clr);
}

void window_progress_t::SetNumbColor(color_t clr) {
    numb.SetColor(clr);
}

void window_progress_t::SetProgressHeight(uint16_t height) {
    if (progr.rect.Height() != height) {
        const Rect16::Height_t h(height);
        progr.rect = h;
        progr.Invalidate();
        numb.rect = (rect - h).Height();
        numb.Invalidate();
    }
}
