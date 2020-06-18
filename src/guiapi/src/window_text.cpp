// window_text.c
#include "window_text.h"
#include "gui.h"

void window_text_init(window_text_t *window) {
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->text = 0;
    window->padding = gui_defaults.padding;
    window->alignment = gui_defaults.alignment;
}

void window_text_draw(window_text_t *window) {
    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        render_text_align(window->win.rect,
            window->text, // @@TODO translate this string here?
            window->font,
            (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_text : window->color_back,
            (window->win.flg & WINDOW_FLG_FOCUSED) ? window->color_back : window->color_text,
            window->padding,
            window->alignment);
        window->win.flg &= ~WINDOW_FLG_INVALID;
    }
}

const window_class_text_t window_class_text = {
    {
        WINDOW_CLS_TEXT,
        sizeof(window_text_t),
        (window_init_t *)window_text_init,
        0,
        (window_draw_t *)window_text_draw,
        0,
    },
};
