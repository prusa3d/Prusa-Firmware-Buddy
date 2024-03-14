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

#include "common/RAII.hpp"

#include "mapi/motion.hpp"
#include "Marlin/src/module/temperature.h"

#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

LOG_COMPONENT_REF(Selftest);
using namespace selftest;

// Feedrate for the extruder moves for MMU FS calibration
const constexpr feedRate_t extruder_fr = 3;
// Move limit for the MMU FS calibration - so that we don't force the filament
// into a cold nozzle in case the sensor fails to detect filament
const constexpr float extruder_move_limit = 70;

CSelftestPart_FSensor::CSelftestPart_FSensor(IPartHandler &state_machine, const FSensorConfig_t &config,
    SelftestFSensor_t &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result)
    , log(1000)
    , log_fast(500)
    , extruder(GetExtruderFSensor(rConfig.extruder_id))
    , side(GetSideFSensor(rConfig.extruder_id)) {
    log_info(Selftest, "%s Started", rConfig.partname);

    if (extruder) {
        extruder->MetricsSetEnabled(true);
    }
    if (side) {
        side->MetricsSetEnabled(true);
    }
    if (!extruder) {
        bsod("%s wrong printer sensor index", rConfig.partname);
    }
}
CSelftestPart_FSensor::~CSelftestPart_FSensor() {
    if (extruder) {
        extruder->MetricsSetEnabled(false);
    }
    if (side) {
        side->MetricsSetEnabled(false);
    }
}

bool CSelftestPart_FSensor::AbortAndInvalidateIfAbortPressed() {
    if (rStateMachine.GetButtonPressed() == Response::Abort_invalidate_test) {
        extruder->SetInvalidateCalibrationFlag();
        if (side) {
            side->SetInvalidateCalibrationFlag();
        }
        log_info(Selftest, "%s user aborted test", rConfig.partname);
        return true;
    }
    return false;
}

LoopResult CSelftestPart_FSensor::state_init() {
#if HAS_TOOLCHANGER()
    if (prusa_toolchanger.is_toolchanger_enabled()) {
        marlin_server::enqueue_gcode("G27 P0 Z5"); // Lift Z if not high enough
        marlin_server::enqueue_gcode_printf("T%d S1 L0 D0", rConfig.extruder_id);
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_wait_tool_pick);
    }
#endif
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_wait_tool_pick() {
#if HAS_TOOLCHANGER()
    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
#endif
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_ask_unload_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_ask_unload);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_ask_unload_wait() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Abort:
        log_error(Selftest, "%s user pressed abort", rConfig.partname);
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

LoopResult CSelftestPart_FSensor::state_filament_unload_enqueue_gcode() {
    if (need_unload) {
        queue.enqueue_one_now("M702 W2"); // unload with return option
        log_info(Selftest, "%s unload enqueued", rConfig.partname);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_filament_unload_confirm_preinit() {
    // TODO set FSM for load dialog - to eliminate flickering
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_unload_confirm);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_filament_unload_wait_finished() {
    // we didn't wanted to unload, so we are not waiting for anything
    if (!need_unload) {
        return LoopResult::RunNext;
    }
    // wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for unload to finish", rConfig.partname);
        return LoopResult::RunCurrent;
    }
    // check if we returned from preheat or finished the unload
    PreheatStatus::Result res = PreheatStatus::ConsumeResult();
    if (res == PreheatStatus::Result::DoneNoFilament) {
        return LoopResult::RunNext;
    }
    return LoopResult::GoToMark0;
}

LoopResult CSelftestPart_FSensor::state_ask_unload_confirm_wait() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Yes:
        // user said yes, there is filament, go back and ask him to remove it
        return LoopResult::GoToMark0;
    case Response::No:
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FSensor::state_calibrate_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_calibrate);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_calibrate() {
    extruder->SetCalibrateRequest(IFSensor::CalibrateRequest::CalibrateNoFilament);
    extruder->Enable(); // must be here for synchronization
    if (side) {
        side->SetCalibrateRequest(IFSensor::CalibrateRequest::CalibrateNoFilament);
        side->Enable(); // must be here for synchronization
    }
    log_info(Selftest, "%s calibrating", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_calibrate_wait_finished() {
    // this state can take long
    // also I do not know what state open after unload
    // so i will make this state soho for a while to avoid flickering
    if (!rStateMachine.WaitSoLastStateIsVisible()) {
        return LoopResult::RunCurrent;
    }

    std::array<fsensor_t, 2> states_to_check = { extruder->Get(), fsensor_t::NoFilament };

    if (!rConfig.mmu_mode) {
        // Try to get side sensor
        if (side) {
            states_to_check[1] = side->Get();
        }
    }

    // Check if we can continue for both filament sensors
    //  fsensor_t::NoFilament continues with check of the other sensor and ends with LoopResult::RunNext
    //  other states return immediately
    for (auto fs_state : states_to_check) {
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

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_insertion_wait_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_insertion_wait);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_insertion_wait() {
    if (AbortAndInvalidateIfAbortPressed()) {
        return LoopResult::Fail;
    }

    std::array<fsensor_t, 2> states_to_check = { extruder->Get(), fsensor_t::HasFilament };

    // TODO this awkward mechanism fails for non-MMU FS Calibration on MK4 with MMU enabled.
    // The rConfig.mmu_mode flag is false, but unless filament is loaded
    // in MMU, FINDA has no filament and the test hence never detects the filament
    if (!rConfig.mmu_mode) {
        // Try to get side sensor
        if (side) {
            states_to_check[1] = side->Get();
        }
    }

    // Check if we can continue for both filament sensors
    //  fsensor_t::HasFilament continues with check of the other sensor and ends with LoopResult::RunNext
    //  other states return immediately
    for (auto fs_state : states_to_check) {
        switch (fs_state) {
        case fsensor_t::HasFilament:
            break;
        case fsensor_t::NoFilament:
            // For MMU filament sensor, turns the extruder to trigger the sensor
            if (rConfig.mmu_mode) {
                if (extruder_moved_amount >= extruder_move_limit) {
                    log_error(Selftest, "%s no filament detected after %.0fmm of extrusion",
                        rConfig.partname, (double)extruder_move_limit);
                    return LoopResult::Fail;
                }
                AutoRestore<bool> CE(thermalManager.allow_cold_extrude);
                thermalManager.allow_cold_extrude = true;
                extruder_moved_amount += mapi::extruder_schedule_turning(extruder_fr); // make extruder turn at 4mm/s
            }
            return LoopResult::RunCurrent;
        default:
            log_error(Selftest, "%s wrong value at insertion check", rConfig.partname);
            IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
            return LoopResult::Fail;
        }
    }

    log_info(Selftest, "%s insertion check has filament", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_insertion_ok_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_insertion_ok);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_insertion_ok() {
    if (AbortAndInvalidateIfAbortPressed()) {
        return LoopResult::Fail;
    }
    if (rStateMachine.GetButtonPressed() == Response::Continue) {
        log_info(Selftest, "%s user confirmed filament is inserted ", rConfig.partname);
        return LoopResult::RunNext;
    }

    IFSensor *const side_local = rConfig.mmu_mode ? nullptr : side;

    // in case that any sensor doesn't think there is filament, go back to insert filament screen
    if (extruder->Get() != fsensor_t::HasFilament) {
        return LoopResult::GoToMark1;
    }

    if (side_local && side_local->Get() != fsensor_t::HasFilament) {
        return LoopResult::GoToMark1;
    }

    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FSensor::state_insertion_calibrate_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_insertion_calibrate);

    log_info(Selftest, "%s calibrating with filament inserted", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_insertion_calibrate_start() {
    extruder->SetCalibrateRequest(IFSensor::CalibrateRequest::CalibrateHasFilament);
    if (side) {
        side->SetCalibrateRequest(IFSensor::CalibrateRequest::CalibrateHasFilament);
    }

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_insertion_calibrate() {
    if (AbortAndInvalidateIfAbortPressed()) {
        return LoopResult::Fail;
    }

    IFSensor *const side_local = rConfig.mmu_mode ? nullptr : side;

    // if calibration not done, wait
    if ((!extruder->IsCalibrationFinished()) || (side && !side->IsCalibrationFinished())) {
        return LoopResult::RunCurrent;
    }

    // after calibration is done, only acceptable state is HasFilament, any other means sensor
    //  disabled itself because he wasn't satisfied with what he read with filament
    if (extruder->Get() != fsensor_t::HasFilament) {
        extruder->SetInvalidateCalibrationFlag();
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
        return LoopResult::Fail;
    }

    if (side_local && side_local->Get() != fsensor_t::HasFilament) {
        side_local->SetInvalidateCalibrationFlag();
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
        return LoopResult::Fail;
    }

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_insertion_calibrate_wait() {
    if (AbortAndInvalidateIfAbortPressed()) {
        return LoopResult::Fail;
    }

    if (!rStateMachine.WaitSoLastStateIsVisible()) {
        return LoopResult::RunCurrent;
    }

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_enforce_remove_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::Fsensor_enforce_remove);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_enforce_remove_mmu_move() {
    if (extruder->Get() == fsensor_t::HasFilament) {
        // For MMU filament sensor - move back the same amount we moved forward
        if (rConfig.mmu_mode) {
            AutoRestore<bool> CE(thermalManager.allow_cold_extrude);
            thermalManager.allow_cold_extrude = true;
            mapi::extruder_move(-extruder_moved_amount, extruder_fr);
        }
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FSensor::state_enforce_remove() {
    if (AbortAndInvalidateIfAbortPressed()) {
        return LoopResult::Fail;
    }

    const fsensor_t new_fs_state = extruder->Get();

    switch (new_fs_state) {
    case fsensor_t::HasFilament:
        rResult.inserted = true;
        return LoopResult::RunCurrent;
    case fsensor_t::NoFilament:
        rResult.inserted = false;
        IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_done);
        return LoopResult::RunNext; // OK - user removed filament
    default:
        break;
    }
    // should never happen
    log_error(Selftest, "%s wrong value after ok", rConfig.partname);
    IPartHandler::SetFsmPhase(PhasesSelftest::FSensor_fail);
    return LoopResult::Fail;
}
