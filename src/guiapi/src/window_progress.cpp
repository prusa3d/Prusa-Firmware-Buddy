// window_progress.c
#include "window_progress.hpp"
#include "gui.hpp"
#include <algorithm>

#define WINDOW_PROGRESS_MAX_TEXT 16

/*****************************************************************************/
//window_numberless_progress_t
window_numberless_progress_t::window_numberless_progress_t(window_t *parent, rect_ui16_t rect, color_t cl_progress, color_t cl_back)
    : window_t(parent, rect)
    , color_progress(cl_progress) {
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
    rect_ui16_t rc = rect;
    const uint16_t progress_w = std::min(progress_in_pixels, rc.w);
    rc.x += progress_w;
    rc.w -= progress_w;
    if (rc.w)
        display::FillRect(rc, color_back);
    rc.x = rect.x;
    rc.w = progress_w;
    if (rc.w)
        display::FillRect(rc, color_progress);
}

/*****************************************************************************/
//window_progress_t
void window_progress_t::SetValue(float val) {
    const float value = std::max(min, std::min(val, max));
    numb.SetValue(value);
    progr.SetProgress((value * progr.rect.w) / max);
}

window_progress_t::window_progress_t(window_t *parent, rect_ui16_t rect, uint16_t h_progr, color_t cl_progress, color_t cl_back)
    : window_frame_t(parent, rect)
    , progr(this, { rect.x, rect.y, rect.w, h_progr }, cl_progress, cl_back)
    , numb(this, { rect.x, uint16_t(rect.y + h_progr), rect.w, uint16_t(rect.h - h_progr) })
    , min(0)
    , max(100) {
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
    if (progr.rect.h != height) {
        progr.rect.h = height;
        progr.Invalidate();
        numb.rect.h = rect.h - height;
        numb.Invalidate();
    }
}
