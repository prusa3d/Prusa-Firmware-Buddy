
/// window_arrows.cpp
#include "window_arrows.hpp"
#include "gui.hpp"
#include "guitypes.hpp"

/// must be same size
constexpr png::Id WindowArrows::id_res_grey_up = { PNG::arrow_up_12x12 };
constexpr png::Id WindowArrows::id_res_grey_down = { PNG::arrow_down_12x12 };
constexpr png::Id WindowArrows::id_res_orange_up = { PNG::arrow_up_orange_12x12 };
constexpr png::Id WindowArrows::id_res_orange_down = { PNG::arrow_down_orange_12x12 };

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
    png::Id id_res1;
    png::Id id_res2;
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
