/**
 * @file selftest_fsensor.h
 * @author Radek Vana
 * @brief part of selftest testing filament sensor
 * @date 2021-11-18
 */

#pragma once

#include <inttypes.h>
#include "i_selftest_part.hpp"
#include "algorithm_range.hpp"
#include "selftest_fsensor_config.hpp"
#include "selftest_fsensor_type.hpp"
#include "filament_sensors_handler.hpp" // fsensor_t
#include "selftest_log.hpp"

namespace selftest {

class CSelftestPart_FSensor {
    IPartHandler &rStateMachine;
    const FSensorConfig_t &rConfig;
    SelftestFSensor_t &rResult;
    int32_t val_no_filament;
    LogTimer log;
    LogTimer log_fast;
    IFSensor *const extruder;
    IFSensor *const side;
    bool need_unload = false;
    float extruder_moved_amount = 0;

    bool AbortAndInvalidateIfAbortPressed();

public:
    CSelftestPart_FSensor(IPartHandler &state_machine, const FSensorConfig_t &config,
        SelftestFSensor_t &result);
    ~CSelftestPart_FSensor();

    LoopResult state_init();
    LoopResult state_wait_tool_pick();
    LoopResult stateCycleMark0() { return LoopResult::MarkLoop0; }
    LoopResult state_ask_unload_init();
    LoopResult state_ask_unload_wait();
    LoopResult state_filament_unload_enqueue_gcode();
    LoopResult state_filament_unload_confirm_preinit();
    LoopResult state_filament_unload_wait_finished();
    LoopResult state_ask_unload_confirm_wait();
    LoopResult state_calibrate_init();
    LoopResult state_calibrate();
    LoopResult state_calibrate_wait_finished();
    LoopResult state_insertion_wait_init();
    LoopResult state_insertion_wait();
    LoopResult stateCycleMark1() { return LoopResult::MarkLoop1; }
    LoopResult state_insertion_ok_init();
    LoopResult state_insertion_ok();
    LoopResult state_insertion_calibrate_init();
    LoopResult state_insertion_calibrate_start();
    LoopResult state_insertion_calibrate();
    LoopResult state_insertion_calibrate_wait(); // wait to be visible
    LoopResult state_enforce_remove_init();
    LoopResult state_enforce_remove_mmu_move();
    LoopResult state_enforce_remove();
};

}; // namespace selftest
