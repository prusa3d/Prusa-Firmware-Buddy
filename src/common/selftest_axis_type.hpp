/**
 * @file selftest_axis_type.hpp
 * @author Radek Vana
 * @brief selftest axis data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestSingleAxis_t {
    uint8_t progress;
    SelftestSubtestState_t state;

    constexpr SelftestSingleAxis_t(uint8_t progress = 0, SelftestSubtestState_t state = SelftestSubtestState_t::undef)
        : progress(progress)
        , state(state) {}

    constexpr bool operator==(const SelftestSingleAxis_t &other) const {
        return (progress == other.progress) && (state == other.state);
    }

    constexpr bool operator!=(const SelftestSingleAxis_t &other) const {
        return !((*this) == other);
    }

    void Pass() {
        state = SelftestSubtestState_t::ok;
        progress = 100;
    }
    void Fail() {
        state = SelftestSubtestState_t::not_good;
        progress = 100;
    }
    void Abort() {} // currently not needed
};

struct SelftestAxis_t {
    uint8_t x_progress;
    uint8_t y_progress;
    uint8_t z_progress;
    SelftestSubtestState_t x_state;
    SelftestSubtestState_t y_state;
    SelftestSubtestState_t z_state;
    uint8_t axis; ///< Currently testing this axis, >Z_AXIS means testing all axes at once

    constexpr SelftestAxis_t(SelftestSingleAxis_t x = SelftestSingleAxis_t(),
        SelftestSingleAxis_t y = SelftestSingleAxis_t(), SelftestSingleAxis_t z = SelftestSingleAxis_t(), uint8_t axis = 0)
        : x_progress(x.progress)
        , y_progress(y.progress)
        , z_progress(z.progress)
        , x_state(x.state)
        , y_state(y.state)
        , z_state(z.state)
        , axis(axis) {}

    constexpr SelftestAxis_t(fsm::PhaseData new_data)
        : SelftestAxis_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { x_progress, y_progress, z_progress, uint8_t(uint8_t(x_state) | (uint8_t(y_state) << 2) | (uint8_t(z_state) << 4) | (axis << 6)) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        x_progress = new_data[0];
        y_progress = new_data[1];
        z_progress = new_data[2];
        x_state = SelftestSubtestState_t(new_data[3] & 0x03);
        y_state = SelftestSubtestState_t((new_data[3] >> 2) & 0x03);
        z_state = SelftestSubtestState_t((new_data[3] >> 4) & 0x03);
        axis = (new_data[3] >> 6) & 0x03;
    }

    constexpr bool operator==(const SelftestAxis_t &other) const {
        return (x_progress == other.x_progress) && (y_progress == other.y_progress) && (z_progress == other.z_progress) && (x_state == other.x_state) && (y_state == other.y_state) && (z_state == other.z_state) && (axis == other.axis);
    }

    constexpr bool operator!=(const SelftestAxis_t &other) const {
        return !((*this) == other);
    }
};
