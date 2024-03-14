/**
 * @file selftest_firstlayer.cpp
 */

#include "selftest_firstlayer.hpp"
#include "wizard_config.hpp"
#include "i_selftest.hpp"
#include "filament_sensors_handler.hpp"
#include "filament.hpp"
#include "../../Marlin/src/gcode/queue.h"
#include "../../lib/Marlin/Marlin/src/gcode/lcd/M73_PE.h"
#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../Marlin/src/module/probe.h"
#include "../../Marlin/src/module/temperature.h"
#include "../../marlin_stubs/G26.hpp"
#include "M70X.hpp"
#include <config_store/store_instance.hpp>

#include <array>

using namespace selftest;
LOG_COMPONENT_REF(Selftest);

CSelftestPart_FirstLayer::CSelftestPart_FirstLayer(IPartHandler &state_machine, const FirstLayerConfig_t &config,
    SelftestFirstLayer_t &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result)
    , state_selected_by_user(StateSelectedByUser::Calib)
    , log(1000) {
    rStateMachine.SetTimeToShowResult(0);
}

LoopResult CSelftestPart_FirstLayer::stateStart() {
    log_info(Selftest, "%s Started", rConfig.partname);

    return LoopResult::RunNext;
}

enum class filament_status {
    TypeUnknown_SensorNoFilament = 0b00, // filament type is not stored in the eeprom, filament sensor is enabled and it does not detect a filament
    TypeKnown_SensorNoFilament = 0b01, // filament type stored in the eeprom,        filament sensor is enabled and it does not detect a filament
    TypeUnknown_SensorValid = 0b10, // filament type is not stored in the eeprom, filament sensor is enabled and it detects a filament or it is not enabled
    TypeKnown_SensorValid = 0b11 // filament type is stored in the eeprom,     filament sensor is enabled and it detects a filament or it is not enabled
};

static filament_status get_filament_status() {
    auto filament = config_store().get_filament_type(0); // first layer calib is on single tool printers only, so should be fine

    uint8_t eeprom = filament != filament::Type::NONE ? static_cast<uint8_t>(filament_status::TypeKnown_SensorNoFilament) : uint8_t(0); // set eeprom flag
    uint8_t sensor = FSensors_instance().GetPrimaryRunout() != fsensor_t::NoFilament ? static_cast<uint8_t>(filament_status::TypeUnknown_SensorValid) : uint8_t(0); // set sensor flag
    return static_cast<filament_status>(eeprom | sensor); // combine flags
}

/**
 * @brief initialization for state which will ask user what to do with filament
 * behavior depends on eeprom and filament sensor
 *
 * @return LoopResult
 */
LoopResult CSelftestPart_FirstLayer::stateAskFilamentInit() {
    filament_status filament = get_filament_status();
    switch (filament) {
    case filament_status::TypeKnown_SensorValid: // do not allow load
        rStateMachine.SetFsmPhase(PhasesSelftest::FirstLayer_filament_known_and_not_unsensed);
        rResult.preselect_response = Response::Next;
        break;
    case filament_status::TypeKnown_SensorNoFilament: // allow load, prepick UNLOAD, force ask preheat
        rStateMachine.SetFsmPhase(PhasesSelftest::FirstLayer_filament_not_known_or_unsensed);
        rResult.preselect_response = Response::Unload;
        break;
    case filament_status::TypeUnknown_SensorNoFilament: // allow load, prepick LOAD, force ask preheat
    case filament_status::TypeUnknown_SensorValid: // most likely same as TypeUnknown_SensorNoFilament, but user inserted filament into sensor
    default:
        rStateMachine.SetFsmPhase(PhasesSelftest::FirstLayer_filament_not_known_or_unsensed);
        rResult.preselect_response = Response::Load;
        break;
    }
    log_info(Selftest, "%s user asked about filament");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateAskFilament() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Next: {
        filament_status filament = get_filament_status();
        if (filament == filament_status::TypeKnown_SensorValid) {
            state_selected_by_user = StateSelectedByUser::Calib;
        } else {
            state_selected_by_user = StateSelectedByUser::Preheat;
        }
        log_info(Selftest, "%s user pressed Next", rConfig.partname);
        return LoopResult::RunNext;
    }
    case Response::Load:
        state_selected_by_user = StateSelectedByUser::Load;
        log_info(Selftest, "%s user pressed Load", rConfig.partname);
        return LoopResult::RunNext;
    case Response::Unload:
        state_selected_by_user = StateSelectedByUser::Unload;
        log_info(Selftest, "%s user pressed Unload", rConfig.partname);
        return LoopResult::RunNext;

    default:
        break;
    }
    return LoopResult::RunCurrent;
}

/*****************************************************************************/
// Preheat
LoopResult CSelftestPart_FirstLayer::statePreheatEnqueueGcode() {
    if (state_selected_by_user != StateSelectedByUser::Preheat) {
        return LoopResult::RunNext;
    }

    // preheat, no return no cooldown, set filament
    return enqueueGcode("M1700 W0 S") ? LoopResult::RunNext : LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::statePreheatWaitFinished() {
    // we didn't wanted to preheat, so we are not waiting for anything
    if (state_selected_by_user != StateSelectedByUser::Preheat) {
        return LoopResult::RunNext;
    }
    // wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for preheat to finish", rConfig.partname);
        return LoopResult::RunCurrent;
    }

    // in case it flickers, we might need to add change of state
    // IPartHandler::SetFsmPhase(PhasesSelftest::);
    return LoopResult::RunNext;
}

/*****************************************************************************/
// Load
LoopResult CSelftestPart_FirstLayer::stateFilamentLoadEnqueueGcode() {
    if (state_selected_by_user != StateSelectedByUser::Load) {
        return LoopResult::RunNext;
    }

    // load, no return no cooldown
    return enqueueGcode("M701 W0") ? LoopResult::RunNext : LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateFilamentLoadWaitFinished() {
    // we didn't wanted to load, so we are not waiting for anything
    if (state_selected_by_user != StateSelectedByUser::Load) {
        return LoopResult::RunNext;
    }
    // wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for load to finish", rConfig.partname);
        return LoopResult::RunCurrent;
    }
    // check if we returned from preheat or finished the load
    switch (PreheatStatus::ConsumeResult()) {
    case PreheatStatus::Result::DoneNoFilament:
        // in case it flickers, we might need to add change of state
        // IPartHandler::SetFsmPhase(PhasesSelftest::);
        return LoopResult::RunNext;
    case PreheatStatus::Result::DoneHasFilament:
        state_selected_by_user = StateSelectedByUser::Calib;
        return LoopResult::RunNext;
    default:
        break;
    }

    // Retry. Something went wrong. Probably user pressed abort
    return LoopResult::GoToMark0;
}

/*****************************************************************************/
// Unload
LoopResult CSelftestPart_FirstLayer::stateFilamentUnloadEnqueueGcode() {
    if (state_selected_by_user != StateSelectedByUser::Unload) {
        return LoopResult::RunNext;
    }

    // change, with return, ask filament type if not known
    return enqueueGcode("M1600 R U1") ? LoopResult::RunNext : LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateFilamentUnloadWaitFinished() {
    // we didn't wanted to unload, so we are not waiting for anything
    if (state_selected_by_user != StateSelectedByUser::Unload) {
        return LoopResult::RunNext;
    }
    // wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for unload to finish", rConfig.partname);
        return LoopResult::RunCurrent;
    }

    // in case it flickers, we might need to add change of state
    // IPartHandler::SetFsmPhase(PhasesSelftest::);

    switch (PreheatStatus::ConsumeResult()) {
    case PreheatStatus::Result::DoneNoFilament:
    case PreheatStatus::Result::Aborted:
    case PreheatStatus::Result::DidNotFinish:
        return LoopResult::GoToMark0;
    default:
        break;
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateShowCalibrateMsg() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_calib);
    return LoopResult::RunNext;
}

static constexpr int axis_steps_per_unit[] = DEFAULT_AXIS_STEPS_PER_UNIT;
static constexpr float z_offset_step = 1.0F / float(axis_steps_per_unit[AxisEnum::Z_AXIS]);
static constexpr float nozzle_to_probe[] = NOZZLE_TO_PROBE_OFFSET;
static constexpr float z_offset_def = nozzle_to_probe[AxisEnum::Z_AXIS];

LoopResult CSelftestPart_FirstLayer::stateInitialDistanceInit() {
    float diff = probe_offset.z - z_offset_def;
    if ((diff <= -z_offset_step) || (diff >= z_offset_step)) {
        IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_use_val);
        current_offset_is_default = false;
        rResult.current_offset = probe_offset.z;
    } else {
        current_offset_is_default = true;
    }

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateInitialDistance() {
    if (current_offset_is_default) {
        return LoopResult::RunNext;
    }

    switch (rStateMachine.GetButtonPressed()) {
    case Response::No:
        probe_offset.z = z_offset_def;
        [[fallthrough]];
    case Response::Yes:
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateShowStartPrint() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_start_print);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::statePrintInit() {
    // reset progress
    marlin_server::set_var_sd_percent_done(0);

    IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_mbl);
    auto filament = config_store().get_filament_type(active_extruder);
    auto filament_desc = filament::get_description(filament);
    const int temp_nozzle = filament_desc.nozzle;
    temp_nozzle_preheat = filament_desc.nozzle_preheat;
    temp_bed = filament_desc.heatbed;

    // nozzle temperature preheat
    thermalManager.setTargetHotend(temp_nozzle_preheat, 0);
    marlin_server::set_temp_to_display(temp_nozzle, 0);

    // bed temperature
    thermalManager.setTargetBed(temp_bed);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateWaitNozzle() {
    std::array<char, sizeof("M109 R170")> gcode_buff; // safe to be local variable, it will copied
    snprintf(gcode_buff.begin(), gcode_buff.size(), "M109 R%d", temp_nozzle_preheat);
    return enqueueGcode(gcode_buff.begin()) ? LoopResult::RunNext : LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateWaitBed() {
    std::array<char, sizeof("M190 S100")> gcode_buff; // safe to be local variable, it will be copied
    snprintf(gcode_buff.begin(), gcode_buff.size(), "M190 S%d", temp_bed);
    return enqueueGcode(gcode_buff.begin()) ? LoopResult::RunNext : LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateHome() {
    return enqueueGcode("G28") ? LoopResult::RunNext : LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateMbl() {
    return enqueueGcode("G29") ? LoopResult::RunNext : LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::statePrint() {
    how_many_times_finished = FirstLayer::HowManyTimesFinished();
    return enqueueGcode("G26") ? LoopResult::RunNext : LoopResult::RunCurrent; // draw firstlay
}

LoopResult CSelftestPart_FirstLayer::stateMblFinished() {
    if (how_many_times_finished == FirstLayer::HowManyTimesStarted()) {
        return LoopResult::RunCurrent;
    }

    IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_print);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::statePrintFinished() {
    return (how_many_times_finished == FirstLayer::HowManyTimesFinished()) ? LoopResult::RunCurrent : LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateReprintInit() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_reprint);
    rResult.preselect_response = Response::No;
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateReprint() {
    switch (rStateMachine.GetButtonPressed()) {
    case Response::Yes:
        reprint = true;
        return LoopResult::RunNext;
    case Response::No:
        reprint = false;
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateCleanSheetInit() {
    if (reprint) {
        IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_clean_sheet);
    }
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateCleanSheet() {
    if (!reprint) {
        return LoopResult::RunNext;
    }

    switch (rStateMachine.GetButtonPressed()) {
    case Response::Next:
    case Response::Continue:
        return LoopResult::GoToMark0;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateFinish() {

    // finish
    log_info(Selftest, "%s Finished\n", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateHandleNext() {
    switch (rStateMachine.GetButtonPressed()) {
    case Response::Next:
    case Response::Continue:
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

bool CSelftestPart_FirstLayer::enqueueGcode(const char *gcode) const {
    bool ret = queue.enqueue_one(gcode);
    log_info(Selftest, ret ? "%s %s enqueued" : "%s %s not enqueued", rConfig.partname, gcode);
    return ret;
}
