/**
 * @file selftest_fans_type.hpp
 * @author Radek Vana
 * @brief selftest fans data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"
#include <limits>
#include "inc/MarlinConfig.h"

struct SelftestFan_t {
    uint8_t progress;
    SelftestSubtestState_t state;

    constexpr SelftestFan_t(uint8_t prog = 0, SelftestSubtestState_t st = SelftestSubtestState_t::undef)
        : progress(prog)
        , state(st) {}

    constexpr bool operator==(const SelftestFan_t &other) const {
        return (progress == other.progress) && (state == other.state);
    }

    constexpr bool operator!=(const SelftestFan_t &other) const {
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

struct SelftestFans_t {
    uint8_t tot_progress = 0;
    static constexpr auto BITS_PER_STATE = 2;                            // how many bits to use to encode state
    static constexpr uint32_t STATE_BITMASK = (1 << BITS_PER_STATE) - 1; //< bitmask for storing state

    std::array<SelftestSubtestState_t, HOTENDS> print_fan_state;
    std::array<SelftestSubtestState_t, HOTENDS> heatbreak_fan_state;

    constexpr SelftestFans_t() {}

    SelftestFans_t(SelftestFan_t *print_fan, SelftestFan_t *heatbreak_fan) {
        tot_progress = std::numeric_limits<decltype(tot_progress)>::max();
        for (size_t i = 0; i < HOTENDS; i++) {
            print_fan_state[i] = print_fan[i].state;
            heatbreak_fan_state[i] = heatbreak_fan[i].state;
            tot_progress = std::min(std::min(tot_progress, print_fan[i].progress), heatbreak_fan[i].progress);
        }
    }

    constexpr SelftestFans_t(fsm::PhaseData new_data) {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        // store 2 bits per state, first stored are printfans, next is heatbreak fans

        uint32_t combined_states_print = 0;
        uint32_t combined_states_heatbreak = 0;
        for (size_t i = 0; i < HOTENDS; i++) {
            combined_states_print |= (uint32_t(print_fan_state[i]) & STATE_BITMASK) << (i * BITS_PER_STATE);
            combined_states_heatbreak |= (uint32_t(heatbreak_fan_state[i]) & STATE_BITMASK) << (i * BITS_PER_STATE);
        }
        static_assert(HOTENDS * 2 * BITS_PER_STATE <= 24);

        const uint32_t combined_states = combined_states_print | (combined_states_heatbreak << (HOTENDS * BITS_PER_STATE));
        fsm::PhaseData ret = { { tot_progress, uint8_t(combined_states >> 16), uint8_t(combined_states >> 8), uint8_t(combined_states) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        tot_progress = new_data[0];
        uint32_t combined_states = new_data[1] << 16 | new_data[2] << 8 | new_data[3];

        for (size_t i = 0; i < HOTENDS; i++) {
            // pick 2 bits per state from combined_states
            constexpr size_t FIRST_HEATBREAK = (HOTENDS * BITS_PER_STATE);

            print_fan_state[i] = static_cast<SelftestSubtestState_t>((combined_states >> (i * BITS_PER_STATE)) & STATE_BITMASK);
            heatbreak_fan_state[i] = static_cast<SelftestSubtestState_t>((combined_states >> (FIRST_HEATBREAK + i * BITS_PER_STATE)) & STATE_BITMASK);
        }
    }

    constexpr bool operator==(const SelftestFans_t &other) const {
        return (tot_progress == other.tot_progress) && print_fan_state == other.print_fan_state && heatbreak_fan_state == other.heatbreak_fan_state;
    }

    constexpr bool operator!=(const SelftestFans_t &other) const {
        return !((*this) == other);
    }
};
