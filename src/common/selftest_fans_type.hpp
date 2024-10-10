/**
 * @file selftest_fans_type.hpp
 * @author Radek Vana
 * @brief selftest fans data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include <common/fsm_base_types.hpp>
#include "marlin_server_extended_fsm_data.hpp"
#include "selftest_sub_state.hpp"
#include <limits>
#include "inc/MarlinConfig.h"
#include <option/has_switched_fan_test.h>

#include <span>

struct SelftestFanHotendResult {
    uint8_t progress { 0 };
    SelftestSubtestState_t print_fan_state { SelftestSubtestState_t::undef };
    SelftestSubtestState_t heatbreak_fan_state { SelftestSubtestState_t::undef };
#if HAS_SWITCHED_FAN_TEST()
    SelftestSubtestState_t fans_switched_state { SelftestSubtestState_t::undef };
#endif /* HAS_SWITCHED_FAN_TEST() */

    constexpr bool operator==(SelftestFanHotendResult const &other) const {
        return progress == other.progress && print_fan_state == other.print_fan_state && heatbreak_fan_state == other.heatbreak_fan_state
#if HAS_SWITCHED_FAN_TEST()
            && fans_switched_state == other.fans_switched_state
#endif /* HAS_SWITCHED_FAN_TEST() */
            ;
    }

    constexpr bool operator!=(const SelftestFanHotendResult &other) const {
        return !((*this) == other);
    }

    void Pass() {
        progress = 100;
    }
    void Fail() {
        progress = 100;
    }
    void Abort() {} // currently not needed

    void ResetFanStates() {
        print_fan_state = SelftestSubtestState_t::undef;
        heatbreak_fan_state = SelftestSubtestState_t::undef;
#if HAS_SWITCHED_FAN_TEST()
        fans_switched_state = SelftestSubtestState_t::undef;
#endif /* HAS_SWITCHED_FAN_TEST() */
    }
};

struct SelftestFansResult : public FSMExtendedData {
    uint8_t progress { 0 };
    std::array<SelftestFanHotendResult, HOTENDS> hotend_results;

    constexpr SelftestFansResult() {}

    SelftestFansResult(const std::span<SelftestFanHotendResult> &results) {
        progress = std::numeric_limits<decltype(progress)>::max();
        for (size_t i = 0; i < results.size(); ++i) {
            hotend_results[i] = results[i];
            progress = std::min(progress, results[i].progress);
        }
    }

    constexpr bool operator==(SelftestFansResult const &other) const {
        return hotend_results == other.hotend_results;
    }

    constexpr bool operator!=(const SelftestFansResult &other) const {
        return !((*this) == other);
    }
};
