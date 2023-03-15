/**
 * @file selftest_fsensor.cpp
 * @author Radek Vana
 * @date 2021-11-18
 */

#include "selftest_fsensor.h"
#include "wizard_config.hpp"
#include "marlin_server.hpp"
#include "selftest_log.hpp"
#include "i_selftest.hpp"
#include "filament_sensor_adc.hpp"
#include <climits>
#include "../../Marlin/src/module/stepper.h"
#include "M70X.hpp"
#include <option/has_side_fsensor.h>
#include <option/has_toolchanger.h>

#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

LOG_COMPONENT_REF(Selftest);
using namespace selftest;

CSelftestPart_FSensor::CSelftestPart_FSensor(IPartHandler &state_machine, const FSensorConfig_t &config,
    SelftestFSensor_t &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result)
    , need_unload(false)
    , log(1000)
    , log_fast(500)
    , show_remove(true) {
    log_info(Selftest, "%s Started", rConfig.partname);
}

LoopResult CSelftestPart_FSensor::stateAskHaveFilamentInit() {
#if HAS_TOOLCHANGER()
    if (prusa_toolchanger.is_toolchanger_enabled()) {
        marlin_server_enqueue_gcode_printf("T%d S1", rConfig.extruder_id);
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_wait_tool_pick);
    }
#endif
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateWaitToolPick() {
#if HAS_TOOLCHANGER()
    if (planner.movesplanned() || queue.has_commands_queued()) {
        return LoopResult::RunCurrent;
    }
#endif

    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_ask_have_filament);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateAskHaveFilament() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::No:
        log_info(Selftest, "%s, user abort, no filament", rConfig.partname);
        return LoopResult::Abort;
    case Response::Yes:
        log_info(Selftest, "%s, user has filament", rConfig.partname);
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FSensor::stateAskUnloadInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_ask_unload);
    return LoopResult::MarkLoop;
}

LoopResult CSelftestPart_FSensor::stateAskUnload() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Abort: //Abort is automatic at state machine level, it should never get here
        log_error(Selftest, "%s user pressed abort, code should not reach this place", rConfig.partname);
        return LoopResult::Abort;
    case Response::Unload:
        need_unload = true;
        log_info(Selftest, "%s user pressed unload", rConfig.partname);
        return LoopResult::RunNext;
    case Response::Continue:
        need_unload = false;
        log_info(Selftest, "%s user pressed continue", rConfig.partname);
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FSensor::stateFilamentUnloadEnqueueGcode() {
    if (need_unload) {
        queue.enqueue_one_now("M702 W2"); // unload with return option
        log_info(Selftest, "%s unload enqueued", rConfig.partname);
    } else {
        //must change state here to avoid flickering
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_calibrate);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateFilamentUnloadWaitFinished() {
    //we didn't wanted to unload, so we are not waiting for anything
    if (!need_unload) {
        return LoopResult::RunNext;
    }
    //wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for unload to finish", rConfig.partname);
        return LoopResult::RunCurrent;
    }
    //check if we returned from preheat or finished the unload
    PreheatStatus::Result res = PreheatStatus::ConsumeResult();
    if (res == PreheatStatus::Result::DoneNoFilament) {
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_unload);
        return LoopResult::RunNext;
    } else {
        return LoopResult::GoToMark;
    }
}

LoopResult CSelftestPart_FSensor::stateFilamentUnloadWaitUser() {
    const Response response = rStateMachine.GetButtonPressed();
    if (!need_unload || response == Response::Continue) {
        // just unloaded, show hourglass
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_calibrate);
        return LoopResult::RunNext;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FSensor::stateCalibrate() {
    IFSensor *extruder = GetExtruderFSensor(rConfig.extruder_id);
    if (extruder) {
        extruder->SetCalibrateFlag();
        extruder->Enable(); //must be here for synchronization
    }
    IFSensor *side = GetSideFSensor(rConfig.extruder_id);
    if (side) {
        side->SetCalibrateFlag();
        side->Enable(); //must be here for synchronization
    }
    log_info(Selftest, "%s calibrating", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateCalibrateWaitFinished() {
    // this state can take long
    // also I do not know what state open after unload
    // so i will make this state soho for a while to avoid flickering
    if (!rStateMachine.WaitSoLastStateIsVisible()) {
        return LoopResult::RunCurrent;
    }

    IFSensor *sensor = GetExtruderFSensor(rConfig.extruder_id);
    if (!sensor) {
        log_error(Selftest, "%s wrong printer sensor index", rConfig.partname);
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
        return LoopResult::Fail;
    }
    current_fs_state = sensor->Get();

    // Try to get side sensor
    sensor = GetSideFSensor(rConfig.extruder_id);
    fsensor_t side_fs_state = fsensor_t::NoFilament;
    if (sensor) {
        side_fs_state = sensor->Get();
    }

    // Check if we can continue for both filament sensors
    //  fsensor_t::NoFilament continues with check of the other sensor and ends with LoopResult::RunNext
    //  other states return immediately
    for (auto fs_state : { current_fs_state, side_fs_state }) {
        switch (fs_state) {
        case fsensor_t::NotInitialized:
            LogInfoTimed(log_fast, "%s waiting for initialization", rConfig.partname);
            return LoopResult::RunCurrent;
        case fsensor_t::NoFilament:
            log_info(Selftest, "%s calibrated", rConfig.partname);
            break;
        case fsensor_t::NotCalibrated:
            log_error(Selftest, "%s not calibrated after calibration", rConfig.partname);
            IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
            return LoopResult::Fail;
        case fsensor_t::HasFilament:
            log_error(Selftest, "%s has filament after calibration", rConfig.partname);
            IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
            return LoopResult::Fail;
        case fsensor_t::NotConnected:
            log_error(Selftest, "%s not connected after calibration", rConfig.partname);
            IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
            return LoopResult::Fail;
        case fsensor_t::Disabled:
            log_error(Selftest, "%s disabled after calibration", rConfig.partname);
            IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
            return LoopResult::Fail;
        }
    }

    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_insertion_check);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateInsertionCheck() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Abort_invalidate_test: {
        IFSensor *sensor = GetExtruderFSensor(rConfig.extruder_id);
        if (sensor)
            sensor->SetInvalidateCalibrationFlag();
        sensor = GetSideFSensor(rConfig.extruder_id);
        if (sensor)
            sensor->SetInvalidateCalibrationFlag();
    }
        log_info(Selftest, "%s user aborted instead of inserting filament", rConfig.partname);
        return LoopResult::Abort;
    default:
        break;
    }

    IFSensor *sensor = GetExtruderFSensor(rConfig.extruder_id);
    if (!sensor) {
        log_error(Selftest, "%s wrong printer sensor index", rConfig.partname);
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
        return LoopResult::Fail;
    }
    current_fs_state = sensor->Get();

    // Try to get side sensor
    sensor = GetSideFSensor(rConfig.extruder_id);
    fsensor_t side_fs_state = fsensor_t::HasFilament;
    if (sensor) {
        side_fs_state = sensor->Get();
    }

    // Check if we can continue for both filament sensors
    //  fsensor_t::HasFilament continues with check of the other sensor and ends with LoopResult::RunNext
    //  other states return immediately
    for (auto fs_state : { current_fs_state, side_fs_state }) {
        switch (fs_state) {
        case fsensor_t::HasFilament:
            break;
        case fsensor_t::NoFilament:
            return LoopResult::RunCurrent;
        default:
            log_error(Selftest, "%s wrong value at insertion check", rConfig.partname);
            IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
            return LoopResult::Fail;
        }
    }

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateInsertionOkInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_insertion_ok);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateInsertionOk() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Abort_invalidate_test: {
        IFSensor *sensor = GetExtruderFSensor(rConfig.extruder_id);
        if (sensor)
            sensor->SetInvalidateCalibrationFlag();
        sensor = GetSideFSensor(rConfig.extruder_id);
        if (sensor)
            sensor->SetInvalidateCalibrationFlag();
    }
        log_info(Selftest, "%s invalidated by user and aborted", rConfig.partname);
        return LoopResult::Abort;
    case Response::Continue:
        log_info(Selftest, "%s finished OK", rConfig.partname);
        return LoopResult::RunNext;
    default:
        break;
    }

    IFSensor *sensor = GetExtruderFSensor(rConfig.extruder_id);
    if (!sensor) {
        log_error(Selftest, "%s wrong printer sensor index", rConfig.partname);
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
        return LoopResult::Fail;
    }
    const fsensor_t new_fs_state = sensor->Get();

    if (new_fs_state == current_fs_state) {
        return LoopResult::RunCurrent;
    }

    current_fs_state = new_fs_state;
    switch (current_fs_state) {
    case fsensor_t::HasFilament:
        show_remove = true;
        rResult.inserted = true;
        return LoopResult::RunCurrent;
    case fsensor_t::NoFilament:
        rResult.inserted = false;
        show_remove = false;
        return LoopResult::RunCurrent;
    default:
        break;
    }
    //should never happen
    log_error(Selftest, "%s wrong value after ok", rConfig.partname);
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
    return LoopResult::Fail;
}

LoopResult CSelftestPart_FSensor::stateEnforceRemoveInit() {
    if (show_remove) {
        IPartHandler::SetFsmPhase(PhasesSelftest::Fsensor_enforce_remove);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::stateEnforceRemove() {
    if (!show_remove) {
        return LoopResult::RunNext; // OK - there is no filament in fsensor
    }
    IFSensor *sensor = GetExtruderFSensor(rConfig.extruder_id);

    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Abort_invalidate_test:
        if (sensor)
            sensor->SetInvalidateCalibrationFlag();
        sensor = GetSideFSensor(rConfig.extruder_id);
        if (sensor)
            sensor->SetInvalidateCalibrationFlag();
        log_info(Selftest, "%s invalidated by user and aborted", rConfig.partname);
        return LoopResult::Abort;
    default:
        break;
    }

    if (!sensor) {
        log_error(Selftest, "%s wrong printer sensor index", rConfig.partname);
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
        return LoopResult::Fail;
    }
    const fsensor_t new_fs_state = sensor->Get();

    switch (new_fs_state) {
    case fsensor_t::HasFilament:
        rResult.inserted = true;
        return LoopResult::RunCurrent;
    case fsensor_t::NoFilament:
        rResult.inserted = false;
        return LoopResult::RunNext; // OK - user removed filament
    default:
        break;
    }
    //should never happen
    log_error(Selftest, "%s wrong value after ok", rConfig.partname);
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
    return LoopResult::Fail;
}
