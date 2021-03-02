/**
 * @file selftest_fans_type.hpp
 * @author Radek Vana
 * @brief selftest fans data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "wizard_config.hpp" // SelftestSubtestState_t

struct SelftestFans_t {
    uint8_t print_fan_progress;  //fan0
    uint8_t nozzle_fan_progress; //fan1
    SelftestSubtestState_t print_fan_state;
    SelftestSubtestState_t nozzle_fan_state;

    constexpr SelftestFans_t(uint8_t prt_fan_prog = 0, uint8_t noz_fan_prog = 0,
        SelftestSubtestState_t prt_fan_st = SelftestSubtestState_t::undef,
        SelftestSubtestState_t noz_fan_st = SelftestSubtestState_t::undef)
        : print_fan_progress(prt_fan_prog)
        , nozzle_fan_progress(noz_fan_prog)
        , print_fan_state(prt_fan_st)
        , nozzle_fan_state(noz_fan_st) {}

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { print_fan_progress, nozzle_fan_progress, uint8_t(print_fan_state), uint8_t(nozzle_fan_state), 0 } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        print_fan_progress = new_data[0];
        nozzle_fan_progress = new_data[1];
        print_fan_state = SelftestSubtestState_t(new_data[2]);
        nozzle_fan_state = SelftestSubtestState_t(new_data[3]);
    }
};
