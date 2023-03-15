#pragma once

#include "i_selftest_part.hpp"
#include "selftest_kennel_config.hpp"
#include "Marlin/src/module/planner.h"
#include "module/prusa/toolchanger.h"

namespace selftest {

class CSelftestPart_Kennel {
public:
    CSelftestPart_Kennel(IPartHandler &state_machine, const KennelConfig_t &config, SelftestKennel_t &result);
    ~CSelftestPart_Kennel();

    LoopResult state_ask_user_needs_calibration();
    LoopResult state_initiate_manual_park();
    LoopResult state_wait_user_manual_park1();
    LoopResult state_wait_user_manual_park2();
    LoopResult state_wait_user_manual_park3();
    LoopResult state_wait_user();
    LoopResult state_initiate_pin_removal();
    LoopResult state_wait_moves_done();
    LoopResult state_ask_user_remove_pin();
    LoopResult state_ask_user_loosen_pillar();
    LoopResult state_ask_user_lock_tool();
    LoopResult state_hold_position();
    LoopResult state_ask_user_tighten_pillar();
    LoopResult state_measure();
    LoopResult state_compute_position();
    LoopResult state_ask_user_install_pins();
    LoopResult state_ask_user_tighten_screw();
    LoopResult state_selftest_entry();
    LoopResult state_selftest_park();
    LoopResult state_selftest_pick();
    LoopResult state_selftest_leave();
    LoopResult state_selftest_save_calibration();

private:
    static constexpr auto NUM_PARK_PICK_CYCLES = 3;
    static constexpr auto EXTRA_SCREWDRIVER_SPACE_MM = 150;
    static constexpr auto X_UNLOCK_DISTANCE_MM = -10;

    IPartHandler &state_machine;
    const KennelConfig_t &config;
    SelftestKennel_t &result;
    bool needs_manual_park;
    xy_long_t position_before_measure;
    PrusaToolInfo old_tool_calibration;
    uint8_t remaining_park_unpark_cycles = NUM_PARK_PICK_CYCLES;
    void revert_tool_info();
};

};
