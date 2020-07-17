// window_progress.c
#include "window_progress.hpp"
#include "gui.hpp"
#include <algorithm>

#define WINDOW_PROGRESS_MAX_TEXT 16

void window_progress_init(window_progress_t *window) {
    /*  window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->color_progress = COLOR_LIME;
    window->font = gui_defaults.font;
    window->padding = gui_defaults.padding;
    window->alignment = ALIGN_CENTER;
    window->height_progress = 8;
    window->format = "%.0f%%";
    window->value = 0;
    window->min = 0;
    window->max = 100;*/
}

void window_progress_draw(window_progress_t *window) {
    /*  if (((window->flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        rect_ui16_t rc = window->rect;
        char text[WINDOW_PROGRESS_MAX_TEXT];
        snprintf(text, WINDOW_PROGRESS_MAX_TEXT, window->format, (double)window->value);
        int progress_w = (int)(rc.w * (window->value - window->min) / (window->max - window->min));
        rc.h = window->height_progress;

        if (1) { // TODO: border attribute, default is no border (Prusa GUI)
            rc.x += progress_w;
            rc.w -= progress_w;
            display::FillRect(rc, window->color_back);
            rc.x = window->rect.x;
            rc.w = progress_w;
            display::FillRect(rc, window->color_progress);
        } else {
            display::FillRect(rc, window->color_progress);
            rc.x += progress_w;
            rc.w = window->rect.w - progress_w;
            display::DrawRect(rc, window->color_progress);
            rc = rect_ui16_sub_padding_ui8(rc, padding_ui8(1, 1, 1, 1));
            display::FillRect(rc, window->color_back);
        }

        rc = window->rect;
        if (rc.h > window->height_progress) {
            rc.y += window->height_progress;
            rc.h -= window->height_progress;
            render_text_align(rc,
                // this MakeRAM is safe - render_text finishes its work and the local string text[] is then no longer needed
                string_view_utf8::MakeRAM((const uint8_t *)text),
                window->font,
                window->color_back,
                window->color_text,
                window->padding,
                window->alignment);
        }
        window->Validate();;
    }*/
}

/*****************************************************************************/
//window_numberless_progress_t
window_numberless_progress_t::window_numberless_progress_t(window_t *parent, window_t *prev, rect_ui16_t rect, color_t cl_progress, color_t cl_back)
    : window_t(parent, prev, rect)
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
    progr.SetProgress((max - min));
}

window_progress_t::window_progress_t(window_t *parent, window_t *prev, rect_ui16_t rect, uint16_t h_progr, color_t cl_progress, color_t cl_back)
    : window_frame_t(&progr, parent, prev, rect)
    , progr(this, nullptr, { rect.x, rect.y, rect.w, h_progr }, cl_progress, cl_back)
    , numb(this, &progr, { rect.x + h_progr, rect.y, rect.w, rect.h - h_progr })
    , min(0)
    , max(100) {
    numb.format = "%.0f%%";
    numb.SetBackColor(cl_back);
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
