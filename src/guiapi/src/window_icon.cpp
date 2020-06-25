// window_icon.c
#include "window_icon.hpp"
#include "gui.hpp"

void window_icon_init(window_icon_t *window) {
    window->color_back = COLOR_BLACK;
    window->id_res = 0;
    window->alignment = ALIGN_CENTER;
}

void window_icon_draw(window_icon_t *window) {
    if (window->flg & WINDOW_FLG_INVALID) {
        //point_ui16_t pt = {window->rect.x, window->rect.y};
        //display::DrawIcon(pt, window->id_res, window->color_back, (window->flg & WINDOW_FLG_FOCUSED)?ROPFN_SWAPBW:0);
        uint8_t ropfn = 0;
        if ((window->flg & WINDOW_FLG_DISABLED)) { // that could not be set, but what if
            ropfn |= ROPFN_DISABLE;
        }
        if ((window->flg & WINDOW_FLG_FOCUSED)) {
            ropfn |= ROPFN_SWAPBW;
        }

        render_icon_align(window->rect, window->id_res, window->color_back,
            RENDER_FLG(window->alignment, ropfn));
        window->flg &= ~WINDOW_FLG_INVALID;
    }
}

const window_class_icon_t window_class_icon = {
    {
        WINDOW_CLS_ICON,
        sizeof(window_icon_t),
        (window_init_t *)window_icon_init,
        0,
        (window_draw_t *)window_icon_draw,
        0,
    },
};

void window_icon_t::SetIdRes(int16_t id) {
    id_res = id;
    _window_invalidate(this);
}
