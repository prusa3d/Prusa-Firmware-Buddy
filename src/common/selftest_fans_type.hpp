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
    uint8_t print_fan_progress;  //fan0
    uint8_t nozzle_fan_progress; //fan1
    uint8_t tot_progress;
    SelftestSubtestState_t print_fan_state;
    SelftestSubtestState_t nozzle_fan_state;

    constexpr SelftestFans_t(uint8_t prt_fan_prog = 0, uint8_t noz_fan_prog = 0, uint8_t tot_prog = 0,
        SelftestSubtestState_t prt_fan_st = SelftestSubtestState_t::undef,
        SelftestSubtestState_t noz_fan_st = SelftestSubtestState_t::undef)
        : print_fan_progress(prt_fan_prog)
        , nozzle_fan_progress(noz_fan_prog)
        , tot_progress(tot_prog)
        , print_fan_state(prt_fan_st)
        , nozzle_fan_state(noz_fan_st) {}

    constexpr SelftestFans_t(fsm::PhaseData new_data)
        : SelftestFans_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { print_fan_progress, nozzle_fan_progress, tot_progress,
            uint8_t(uint8_t(print_fan_state) | (uint8_t(nozzle_fan_state) << 2)) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        print_fan_progress = new_data[0];
        nozzle_fan_progress = new_data[1];
        tot_progress = new_data[2];
        print_fan_state = SelftestSubtestState_t(new_data[3] & 0x03);
        nozzle_fan_state = SelftestSubtestState_t((new_data[3] >> 2) & 0x03);
    }

    constexpr bool operator==(const SelftestFans_t &other) const {
        return (print_fan_progress == other.print_fan_progress) && (nozzle_fan_progress == other.nozzle_fan_progress) && (tot_progress == other.tot_progress) && (print_fan_state == other.print_fan_state) && (nozzle_fan_state == other.nozzle_fan_state);
    }

    constexpr bool operator!=(const SelftestFans_t &other) const {
        return !((*this) == other);
    }
};
