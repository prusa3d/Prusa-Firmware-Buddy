/**
 * @file window_wizard_icon.hpp
 */

#pragma once

#include <window.hpp>
#include <selftest_sub_state.hpp>
#include <config_store/constants.hpp>
#include <bitset>
#include <array>

class WindowIconOkNgArray : public window_t {

public:
    constexpr static uint8_t max_icon_cnt = config_store_ns::max_tool_count;
    constexpr static uint8_t icon_space_width = 20;

    WindowIconOkNgArray(window_t *parent, const point_i16_t pt, uint8_t icon_cnt = 1, const SelftestSubtestState_t state = SelftestSubtestState_t::undef);
    SelftestSubtestState_t GetState(const size_t idx = 0) const { return states[idx]; }
    void SetState(const SelftestSubtestState_t s, const size_t idx = 0);
    void SetIconHidden(const size_t idx, const bool hidden);
    void SetIconCount(const size_t new_icon_cnt, const Rect16 new_rect);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    std::array<SelftestSubtestState_t, max_icon_cnt> states;
    std::bitset<max_icon_cnt> hidden;
    uint8_t icon_cnt;
    uint8_t animation_stage;
};

using WindowIcon_OkNg = WindowIconOkNgArray;
