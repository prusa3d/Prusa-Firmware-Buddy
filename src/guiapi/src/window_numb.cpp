// window_numb.cpp
#include "window_numb.hpp"
#include "gui.hpp"

#define WINDOW_NUMB_MAX_TEXT 16

void window_numb_init(window_numb_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->value = 0;
    window->format = "%.0f";
    window->padding = gui_defaults.padding;
    window->alignment = gui_defaults.alignment;
}

void window_numb_draw(window_numb_t *window) {
    if (((window->flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        color_t clr_back = (window->flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back;
        color_t clr_text = (window->flg & WINDOW_FLG_FOCUSED) ? window->color_back : window->color_text;
        if (window->flg & WINDOW_FLG_CAPTURE)
            clr_text = COLOR_ORANGE;
        char text[WINDOW_NUMB_MAX_TEXT];
        if (window->flg & WINDOW_FLG_NUMB_FLOAT2INT) {
            snprintf(text, WINDOW_NUMB_MAX_TEXT, window->format, (int)(window->value));
        } else {
            snprintf(text, WINDOW_NUMB_MAX_TEXT, window->format, (double)window->value);
        }

        render_text_align(window->rect,
            text, // @@TODO translate this string here?
            window->font,
            clr_back,
            clr_text,
            window->padding,
            window->alignment);
        window->flg &= ~WINDOW_FLG_INVALID;
    }
}

const window_class_numb_t window_class_numb = {
    {
        WINDOW_CLS_NUMB,
        sizeof(window_numb_t),
        (window_init_t *)window_numb_init,
        0,
        (window_draw_t *)window_numb_draw,
        0,
    },
};

void window_numb_t::SetFormat(const char *frmt) {
    format = frmt;
    _window_invalidate(this);
}

//todo use this later
//virtual methods does not work yet - stupid memcpy
/*
void window_numb_t::SetValue(float val) {
    setValue(val);
    _window_invalidate(this);
}
*/

//todo use this virtual methods does not work yet - stupid memcpy
/*
void window_numb_t::setValue(float val) {
    value = val;
}
*/

void window_numb_t::SetValue(float val) {
    value = val;
    _window_invalidate(this);
}
