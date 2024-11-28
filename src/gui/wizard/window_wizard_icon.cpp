/**
 * @file window_wizard_icon.cpp
 */

#include <window_wizard_icon.hpp>
#include <img_resources.hpp>
#include <display_helper.h>

// dash ok and nok must be same size
constexpr const uint16_t icon_w = img::ok_color_18x18.w;
constexpr const uint16_t icon_h = img::ok_color_18x18.h;
constexpr static uint32_t ANIMATION_STEP_MS = 128;

WindowIconOkNgArray::WindowIconOkNgArray(window_t *parent, const point_i16_t pt, uint8_t icon_cnt, const SelftestSubtestState_t state)
    : window_t(parent, Rect16(pt.x, pt.y, icon_cnt * icon_space_width, icon_h))
    , hidden({})
    , icon_cnt(icon_cnt)
    , animation_stage(0) {
    states.fill(state);
    for (uint8_t i = 0; i < max_icon_cnt; i++) {
        if (i >= icon_cnt) {
            hidden.set(i);
        }
    }
}

void WindowIconOkNgArray::SetIconHidden(const size_t idx, const bool set_hidden) {
    assert(idx < max_icon_cnt);
    if (hidden[idx] != set_hidden) {
        hidden[idx] = set_hidden;
        Invalidate();
    }
}

void WindowIconOkNgArray::SetState(const SelftestSubtestState_t state, const size_t idx) {
    assert(idx < max_icon_cnt);
    if (state != states[idx]) {
        states[idx] = state;
        if (!hidden[idx]) {
            Invalidate();
        }
    }
}

void WindowIconOkNgArray::unconditionalDraw() {
    size_t visible_left = 0;
    for (size_t i = 0; i < icon_cnt; i++) {
        if (!hidden[i]) {
            visible_left++;
        }
    }
    for (size_t i = 0; i < icon_cnt; i++) {
        if (hidden[i]) {
            continue;
        }
        const img::Resource *id_res = nullptr;
        switch (GetState(i)) {
        case SelftestSubtestState_t::ok:
            id_res = &img::ok_color_18x18;
            break;
        case SelftestSubtestState_t::not_good:
            id_res = &img::nok_color_18x18;
            break;
        case SelftestSubtestState_t::undef:
            id_res = &img::dash_18x18;
            break;
        case SelftestSubtestState_t::running: {
            id_res = img::spinner_16x16_stages[animation_stage]; // no need to check index out of array range
        } break;
        }

        // Icons are aligned to the right
        const Rect16 icon_rect(GetRect().Right() - visible_left * icon_space_width, GetRect().Top(), id_res->w, id_res->h);
        render_icon_align(icon_rect, id_res, GetBackColor(), icon_flags(Align_t::Center()));
        visible_left--;
    }
}

void WindowIconOkNgArray::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {
    case GUI_event_t::LOOP:
        if (GetState() == SelftestSubtestState_t::running) {
            const uint8_t animation_stage_ = (gui::GetTick() / ANIMATION_STEP_MS) % 4;
            if (animation_stage_ != animation_stage) {
                animation_stage = animation_stage_;
                Invalidate();
            }
        }
        break;
    default:
        break;
    }
}
