
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
        }()) {
    SetState(WindowArrows::State_t::undef);
}

WindowArrows::State_t WindowArrows::GetState() const {
    return static_cast<WindowArrows::State_t>(flags.mem_array_u08[1]);
}

//there is a free space in window_t flags, store state in it
void WindowArrows::SetState(WindowArrows::State_t s) {
    const uint8_t state = static_cast<uint8_t>(s);
    if (state != flags.mem_array_u08[1]) {
        flags.mem_array_u08[1] = state;
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
