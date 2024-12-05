/**
 * @file window_wizard_icon.cpp
 */

#include <window_wizard_icon.hpp>
#include <img_resources.hpp>
#include <display_helper.h>

// dash ok and nok must be same size
constexpr static uint16_t icon_h = img::ok_color_18x18.h;
constexpr static uint32_t animation_step_ms = 128;

WindowIconOkNgArray::WindowIconOkNgArray(window_t *parent, const point_i16_t pt, uint8_t icon_cnt, const SelftestSubtestState_t state)
    : window_t(parent, Rect16(pt.x, pt.y, icon_cnt * icon_space_width, icon_h))
    , hidden({})
    , icon_cnt(icon_cnt)
    , animation_stage(0) {
    assert(icon_cnt <= max_icon_cnt);
    states.fill(state);
}

void WindowIconOkNgArray::SetIconHidden(const size_t idx, const bool set_hidden) {
    assert(idx < icon_cnt);
    if (hidden[idx] != set_hidden) {
        hidden[idx] = set_hidden;
        Invalidate();
    }
}

void WindowIconOkNgArray::SetState(const SelftestSubtestState_t state, const size_t idx) {
    assert(idx < icon_cnt);
    if (state != states[idx]) {
        states[idx] = state;
        if (!hidden[idx]) {
            Invalidate();
        }
    }
}

void WindowIconOkNgArray::SetIconCount(const size_t new_icon_cnt) {
    assert(icon_cnt <= max_icon_cnt);
    if (icon_cnt == new_icon_cnt) {
        return;
    }
    icon_cnt = new_icon_cnt;

    Rect16 new_rect = GetRect();
    new_rect = Rect16::Left_t(new_rect.Right() - new_icon_cnt * icon_space_width);
    new_rect = Rect16::Width_t(new_icon_cnt * icon_space_width);
    SetRect(new_rect);
    Invalidate();
}

void WindowIconOkNgArray::unconditionalDraw() {
    size_t visible_left = icon_cnt - hidden.count();
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

void WindowIconOkNgArray::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::LOOP: {
        const uint8_t animation_stage_ = (gui::GetTick() / animation_step_ms) % 4;
        if (animation_stage_ != animation_stage) {
            animation_stage = animation_stage_;
            Invalidate();
        }

    } break;
    default:
        break;
    }
    GetParent()->WindowEvent(sender, event, param);
}
