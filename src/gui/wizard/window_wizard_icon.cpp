/**
 * @file window_wizard_icon.cpp
 */

#include <unistd.h>
#include "window_wizard_icon.hpp"
#include "img_resources.hpp"

// dash ok and nok must be same size
constexpr const img::Resource &id_res_na = img::dash_18x18;
constexpr const img::Resource &id_res_ok = img::ok_color_18x18;
constexpr const img::Resource &id_res_ng = img::nok_color_18x18;
constexpr const std::array<const img::Resource *, 4> id_res_ip = { { &img::spinner0_16x16, &img::spinner1_16x16, &img::spinner2_16x16, &img::spinner3_16x16 } };

// Icon rect is increased by padding, icon is centered inside it
WindowIcon_OkNg::WindowIcon_OkNg(window_t *parent, point_i16_t pt, SelftestSubtestState_t state, padding_ui8_t padding)
    : AddSuperWindow<window_aligned_t>(
        parent,
        [pt, padding] {
            return Rect16(pt,
                id_res_na.w + padding.left + padding.right,
                id_res_na.h + padding.top + padding.bottom);
        }())
    , state(state) {
    SetAlignment(Align_t::Center());
}

SelftestSubtestState_t WindowIcon_OkNg::GetState() const {
    return state;
}

void WindowIcon_OkNg::SetState(SelftestSubtestState_t s) {
    if (s != state) {
        state = s;
        Invalidate();
    }
}

void WindowIcon_OkNg::unconditionalDraw() {
    const img::Resource *id_res = nullptr;
    switch (GetState()) {
    case SelftestSubtestState_t::ok:
        id_res = &id_res_ok;
        break;
    case SelftestSubtestState_t::not_good:
        id_res = &id_res_ng;
        break;
    case SelftestSubtestState_t::undef:
        id_res = &id_res_na;
        break;
    case SelftestSubtestState_t::running: {
        const size_t blink_state = (flags.blink1 << 1) | flags.blink0; // sets 2 lowest bits guaranted to be 0 .. 3
        id_res = id_res_ip[blink_state]; // no need to check index out of array range
    } break;
    }

    render_icon_align(GetRect(), id_res, GetBackColor(), GetAlignment());
}

void WindowIcon_OkNg::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    if (GetState() == SelftestSubtestState_t::running) {
        bool b0 = (gui::GetTick() / uint32_t(ANIMATION_STEP_MS)) & 0b01;
        bool b1 = (gui::GetTick() / uint32_t(ANIMATION_STEP_MS)) & 0b10;
        if (flags.blink0 != b0 || flags.blink1 != b1) {
            flags.blink0 = b0;
            flags.blink1 = b1;
            Invalidate();
        }
    }
}
