#pragma once

#include <limits>
#include <cassert>
#include <cstring>

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"
#include "inc/MarlinConfig.h"

struct SelftestToolOffsets_t {
    uint8_t progress;
    SelftestSubtestState_t state;

    constexpr SelftestToolOffsets_t(uint8_t prog = 0, SelftestSubtestState_t st = SelftestSubtestState_t::undef)
        : progress(prog)
        , state(st) {}

    constexpr bool operator==(const SelftestToolOffsets_t &other) const {
        return (progress == other.progress) && (state == other.state);
    }

    constexpr bool operator!=(const SelftestToolOffsets_t &other) const {
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

    void Abort() {}

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { static_cast<uint8_t>(state) } };
        return ret;
    }
};
