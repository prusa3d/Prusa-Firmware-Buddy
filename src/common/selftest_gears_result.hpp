#pragma once

#include <common/fsm_base_types.hpp>
#include "selftest_sub_state.hpp"

struct SelftestGearsResult {
    constexpr SelftestGearsResult() {}

    constexpr SelftestGearsResult(fsm::PhaseData new_data)
        : SelftestGearsResult() {
        deserialize(new_data);
    }

    constexpr fsm::PhaseData serialize() const {
        return fsm::PhaseData();
    }

    constexpr void deserialize([[maybe_unused]] fsm::PhaseData new_data) {
    }

    constexpr bool operator==([[maybe_unused]] const SelftestGearsResult &other) const {
        return true;
    }

    constexpr bool operator!=(const SelftestGearsResult &other) const {
        return !((*this) == other);
    }

    void Pass() {}
    void Fail() {}
    void Abort() {} // currently not needed
};
