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
    fsensor_t current_fs_state;
    bool need_unload;
    LogTimer log;
    LogTimer log_fast;
    bool show_remove;

public:
    CSelftestPart_FSensor(IPartHandler &state_machine, const FSensorConfig_t &config,
        SelftestFSensor_t &result);

    LoopResult stateAskHaveFilamentInit();
    LoopResult stateWaitToolPick();
    LoopResult stateAskHaveFilament();
    LoopResult stateAskUnloadInit();
    LoopResult stateAskUnload();
    LoopResult stateFilamentUnloadEnqueueGcode();
    LoopResult stateFilamentUnloadWaitFinished();
    LoopResult stateFilamentUnloadWaitUser();
    LoopResult stateCalibrate();
    LoopResult stateCalibrateWaitFinished();
    LoopResult stateInsertionCheck();
    LoopResult stateInsertionOkInit();
    LoopResult stateInsertionOk();
    LoopResult stateEnforceRemoveInit();
    LoopResult stateEnforceRemove();
};

};
