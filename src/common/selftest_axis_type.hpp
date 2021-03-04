/**
 * @file selftest_axis_type.hpp
 * @author Radek Vana
 * @brief selftest axis data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestAxis_t {
    uint8_t x_progress;
    uint8_t y_progress;
    uint8_t z_progress;
    SelftestSubtestState_t x_state;
    SelftestSubtestState_t y_state;
    SelftestSubtestState_t z_state;

    constexpr SelftestAxis_t(uint8_t x_progress = 0, uint8_t y_progress = 0, uint8_t z_progress = 0,
        SelftestSubtestState_t x_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t y_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t z_state = SelftestSubtestState_t::undef)
        : x_progress(x_progress)
        , y_progress(y_progress)
        , z_progress(z_progress)
        , x_state(x_state)
        , y_state(y_state)
        , z_state(z_state) {}

    constexpr SelftestAxis_t(fsm::PhaseData new_data)
        : SelftestAxis_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { x_progress, y_progress, z_progress, uint8_t(uint8_t(x_state) | (uint8_t(y_state) << 2) | (uint8_t(z_state) << 4)) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        x_progress = new_data[0];
        y_progress = new_data[1];
        z_progress = new_data[2];
        x_state = SelftestSubtestState_t(new_data[3] & 0x03);
        y_state = SelftestSubtestState_t((new_data[3] >> 2) & 0x03);
        z_state = SelftestSubtestState_t((new_data[3] >> 4) & 0x03);
    }

    constexpr bool operator==(const SelftestAxis_t &other) const {
        return (x_progress == other.x_progress) && (y_progress == other.y_progress) && (z_progress == other.z_progress) && (x_state == other.x_state) && (y_state == other.y_state) && (z_state == other.z_state);
    }

    constexpr bool operator!=(const SelftestAxis_t &other) const {
        return !((*this) == other);
    }
};
