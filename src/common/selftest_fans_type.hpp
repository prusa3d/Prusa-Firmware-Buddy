/**
 * @file selftest_fans_type.hpp
 * @author Radek Vana
 * @brief selftest fans data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestFans_t {
    uint8_t print_fan_progress;
    uint8_t heatbreak_fan_progress;
    uint8_t tot_progress;
    SelftestSubtestState_t print_fan_state;
    SelftestSubtestState_t heatbreak_fan_state;

    constexpr SelftestFans_t(uint8_t prt_fan_prog = 0, uint8_t hb_fan_prog = 0, uint8_t tot_prog = 0,
        SelftestSubtestState_t prt_fan_st = SelftestSubtestState_t::undef,
        SelftestSubtestState_t hb_fan_st = SelftestSubtestState_t::undef)
        : print_fan_progress(prt_fan_prog)
        , heatbreak_fan_progress(hb_fan_prog)
        , tot_progress(tot_prog)
        , print_fan_state(prt_fan_st)
        , heatbreak_fan_state(hb_fan_st) {}

    constexpr SelftestFans_t(fsm::PhaseData new_data)
        : SelftestFans_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { print_fan_progress, heatbreak_fan_progress, tot_progress,
            uint8_t(uint8_t(print_fan_state) | (uint8_t(heatbreak_fan_state) << 2)) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        print_fan_progress = new_data[0];
        heatbreak_fan_progress = new_data[1];
        tot_progress = new_data[2];
        print_fan_state = SelftestSubtestState_t(new_data[3] & 0x03);
        heatbreak_fan_state = SelftestSubtestState_t((new_data[3] >> 2) & 0x03);
    }

    constexpr bool operator==(const SelftestFans_t &other) const {
        return (print_fan_progress == other.print_fan_progress) && (heatbreak_fan_progress == other.heatbreak_fan_progress) && (tot_progress == other.tot_progress) && (print_fan_state == other.print_fan_state) && (heatbreak_fan_state == other.heatbreak_fan_state);
    }

    constexpr bool operator!=(const SelftestFans_t &other) const {
        return !((*this) == other);
    }
};
