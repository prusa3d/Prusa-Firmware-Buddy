#pragma once

#include "i_selftest_part.hpp"
#include "selftest_dock_config.hpp"
#include "Marlin/src/module/planner.h"
#include "module/prusa/toolchanger.h"

namespace selftest {

class CSelftestPart_Dock {
public:
    CSelftestPart_Dock(IPartHandler &state_machine, const DockConfig_t &config, SelftestDock_t &result);
    ~CSelftestPart_Dock();

    LoopResult state_ask_user_needs_calibration();
    LoopResult stateMoveAwayInit();
    LoopResult stateMoveAwayWait();
    LoopResult state_initiate_manual_park();
    LoopResult state_cycle_mark0() { return LoopResult::MarkLoop0; }
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
    LoopResult state_ask_user_tighten_screw();
    LoopResult state_ask_user_install_pins();
    LoopResult state_selftest_check_todock();
    LoopResult state_selftest_check_unlock();
    LoopResult state_selftest_check_away();
    LoopResult state_selftest_check_state();
    LoopResult state_selftest_entry();
    LoopResult state_cycle_mark1() { return LoopResult::MarkLoop1; }
    LoopResult state_selftest_pick();
    LoopResult state_selftest_park();
    LoopResult state_selftest_leave();
    LoopResult state_selftest_move_away();
    LoopResult state_selftest_congratulate();
    LoopResult state_selftest_save_calibration();

private:
    static constexpr auto NUM_PARK_PICK_CYCLES = 3;
    static constexpr auto EXTRA_SCREWDRIVER_SPACE_MM = 150;
    static constexpr auto X_UNLOCK_DISTANCE_MM = -10;

    IPartHandler &state_machine;
    const DockConfig_t &config;
    SelftestDock_t &result;
    bool needs_manual_park;
    xy_long_t position_before_measure;
    PrusaToolInfo old_tool_calibration;
    uint8_t remaining_park_unpark_cycles = NUM_PARK_PICK_CYCLES;
    buddy::puppies::Dwarf &dwarf; ///< Reference to the dwarf whose dock is to be calibrated

    bool toolcheck_was_disabled = false; ///< Remember if toolcheck was disabled to not reenable

    /// Disable toolcheck and remember it was disabled
    inline void toolcheck_disable() {
        if (!toolcheck_was_disabled) {
            toolcheck_was_disabled = true;
            prusa_toolchanger.toolcheck_disable();
        }
    }

    /// Safely reenable automatic toolchange
    inline void toolcheck_reenable() {
        if (toolcheck_was_disabled) {
            prusa_toolchanger.toolcheck_enable();
            toolcheck_was_disabled = false;
        }
    }

    struct MoveRequired {
        bool picked; ///< Dwarf should have this picked state
        bool parked; ///< Dwarf should have this parked state
        uint32_t timeout_start; ///< Dwarf should be in the above state a short moment after this time [ms]
        bool timeout_valid; ///< If true, variable timeout_start is valid
    } move_required; ///< Requirements for the current move to be successful

    /**
     * @brief Enable endstops to make a homing move.
     * To do a move that is interrupted as soon as an endstop is hit.
     */
    void prepare_homing();

    void revert_tool_info();
};

}; // namespace selftest
