/**
 * @file crash_recovery_type.hpp
 * @author Radek Vana
 * @brief crash recovery data to be passed between threads
 * @date 2021-10-29
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

class Crash_recovery_fsm {
public:
    SelftestSubtestState_t x;
    SelftestSubtestState_t y;

    constexpr Crash_recovery_fsm(SelftestSubtestState_t x = SelftestSubtestState_t::undef, SelftestSubtestState_t y = SelftestSubtestState_t::undef)
        : x(x)
        , y(y) {}

    constexpr Crash_recovery_fsm(fsm::PhaseData new_data)
        : Crash_recovery_fsm() {
        Deserialize(new_data);
    }

    constexpr void set(SelftestSubtestState_t x_state = SelftestSubtestState_t::undef, SelftestSubtestState_t y_state = SelftestSubtestState_t::undef) {
        x = x_state;
        y = y_state;
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { uint8_t(x), uint8_t(y) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        x = SelftestSubtestState_t(new_data[0]);
        y = SelftestSubtestState_t(new_data[1]);
    }

    bool operator==(const Crash_recovery_fsm &other) const {
        return Serialize() == other.Serialize();
    }

    bool operator!=(const Crash_recovery_fsm &other) const {
        return !((*this) == other);
    }
};
