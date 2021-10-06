
/// window_arrows.cpp
#include "window_arrows.hpp"
#include "gui.hpp"
#include "guitypes.hpp"
#include "resource.h"

/// must be same size
const uint16_t WindowArrows::id_res_grey_up = IDR_PNG_arrow_up_8px;
const uint16_t WindowArrows::id_res_grey_down = IDR_PNG_arrow_down_8px;
const uint16_t WindowArrows::id_res_orange_up = IDR_PNG_arrow_up_orange_8px;
const uint16_t WindowArrows::id_res_orange_down = IDR_PNG_arrow_down_orange_8px;

/// Icon rect is increased by padding, icon is centered inside it
WindowArrows::WindowArrows(window_t *parent, point_i16_t pt, padding_ui8_t padding)
    : window_aligned_t(
        parent,
        [pt, padding] {
            size_ui16_t sz = window_icon_t::CalculateMinimalSize(WindowArrows::id_res_grey_up);
            if (!(sz.h && sz.w))
                return Rect16();
            return Rect16(pt,
                sz.w + padding.left + padding.right,
                sz.h + padding.top + padding.bottom);
        }())
    , state(WindowArrows::State_t::undef) {
}

WindowArrows::State_t WindowArrows::GetState() const {
    return state;
}

void WindowArrows::SetState(WindowArrows::State_t s) {
    if (s != state) {
        state = s;
        Invalidate();
    }
}

void WindowArrows::unconditionalDraw() {
    uint16_t id_res1;
    uint16_t id_res2;
    switch (GetState()) {
    case WindowArrows::State_t::up:
        id_res1 = id_res_orange_up;
        id_res2 = id_res_grey_down;
        break;
    case WindowArrows::State_t::down:
        id_res1 = id_res_grey_up;
        id_res2 = id_res_orange_down;
        break;
    case WindowArrows::State_t::undef:
    default:
        id_res1 = id_res_grey_up;
        id_res2 = id_res_grey_down;
        break;
    }
    render_icon_align(GetRect(), id_res1, GetBackColor(), GetAlignment());
    render_icon_align(GetRect() + Rect16::Top_t(12), id_res2, GetBackColor(), GetAlignment());
}
