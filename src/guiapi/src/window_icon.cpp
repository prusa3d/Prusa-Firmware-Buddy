// window_icon.c
#include "window_icon.hpp"
#include "gui.hpp"

void window_icon_init(window_icon_t *window) {
    window->color_back = COLOR_BLACK;
    window->id_res = 0;
    window->alignment = ALIGN_CENTER;
}

void window_icon_draw(window_icon_t *window) {
    if (window->IsInvalid()) {
        //point_ui16_t pt = {window->rect.x, window->rect.y};
        //display::DrawIcon(pt, window->id_res, window->color_back, (window->IsFocused())?ROPFN_SWAPBW:0);
        uint8_t ropfn = 0;
        if ((window->IsBWSwapped())) { // that could not be set, but what if
            ropfn |= ROPFN_DISABLE;
        }
        if ((window->IsFocused())) {
            ropfn |= ROPFN_SWAPBW;
        }

        render_icon_align(window->rect, window->id_res, window->color_back,
            RENDER_FLG(window->alignment, ropfn));
        window->Validate();
        ;
    }
}

void window_icon_t::SetIdRes(int16_t id) {
    id_res = id;
    Invalidate();
}

window_icon_t::window_icon_t(window_t *parent, window_t *prev, rect_ui16_t rect, uint16_t id_res)
    : window_t(parent, prev, rect)
    , id_res(id_res)
    , alignment(ALIGN_CENTER) {}

void window_icon_t::unconditionalDraw() {
    uint8_t ropfn = 0;
    if ((IsBWSwapped())) { // that could not be set, but what if
        ropfn |= ROPFN_DISABLE;
    }
    if ((IsFocused())) {
        ropfn |= ROPFN_SWAPBW;
    }

    render_icon_align(rect, id_res, color_back, RENDER_FLG(alignment, ropfn));
}

bool window_icon_t::IsBWSwapped() const { return f_parent_defined0 == true; }
void window_icon_t::SwapBW() { f_parent_defined0 = true; }
void window_icon_t::UnswapBW() { f_parent_defined0 = false; }
