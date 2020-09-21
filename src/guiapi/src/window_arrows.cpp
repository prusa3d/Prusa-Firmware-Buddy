
/// window_arrows

/// must be same size
const uint16_t WindowArrows::id_res_grey_up = IDR_PNG_arrow_up;
const uint16_t WindowArrows::id_res_grey_down = IDR_PNG_arrow_down;
const uint16_t WindowArrows::id_res_orange_up = IDR_PNG_arrow_up_orange;
const uint16_t WindowArrows::id_res_orange_down = IDR_PNG_arrow_down_orange;

/// Icon rect is increased by padding, icon is centered inside it
WindowArrows::WindowArrows(window_t *parent, point_i16_t pt, padding_ui8_t padding)
    : window_aligned_t(
        parent,
        [pt, padding] {
            size_ui16_t sz = window_icon_t::CalculateMinimalSize(WindowArrows::id_res_ok);
            if (!(sz.h && sz.w))
                return Rect16();
            return Rect16(pt,
                sz.w + padding.left + padding.right,
                sz.h + padding.top + padding.bottom);
        }()) {
    SetState(State_t::undef);
}
WindowArrows::State_t WindowArrows::GetState() const {
    return static_cast<State_t>(mem_array_u08[1]);
}
//there is a free space in window_t flags, store state in it
void WindowArrows::SetState(State_t s) {
    const uint8_t state = static_cast<uint8_t>(s);
    if (state != mem_array_u08[1]) {
        mem_array_u08[1] = state;
        Invalidate();
    }
}
void WindowArrows::unconditionalDraw() {
    uint16_t id_res;
    switch (GetState()) {
    case State_t::ok:
        id_res = id_res_ok;
        break;
    case State_t::ng:
        id_res = id_res_ng;
        break;
    case State_t::undef:
        id_res = 0;
        break;
    }
    render_icon_align(rect, id_res, color_back, GetAlignment());
}
