/**
 * @file selftest_firstlayer.cpp
 */

#include "selftest_firstlayer.hpp"
#include "wizard_config.hpp"
#include "i_selftest.hpp"
#include "filament_sensor_api.hpp"
#include "filament.hpp"
#include "../../Marlin/src/gcode/queue.h"
#include "../../Marlin/src/module/probe.h"
#include "../../Marlin/src/module/temperature.h"
#include "../../marlin_stubs/G26.hpp"
#include "M70X.hpp"

using namespace selftest;

CSelftestPart_FirstLayer::CSelftestPart_FirstLayer(IPartHandler &state_machine, const FirstLayerConfig_t &config,
    SelftestFirstLayer_t &result)
    : rStateMachine(state_machine)
    , rConfig(config)
    , rResult(result)
    , filament_known_but_unsensed(false)
    , state_selected_by_user(StateSelectedByUser::Calib)
    , log(1000) {
}

LoopResult CSelftestPart_FirstLayer::stateStart() {
    LogInfo("%s Started", rConfig.partname);
    SelftestInstance().log_printf("%s Started\n", rConfig.partname);

    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateAskFilamentInit() {
    uint8_t filament = 0;
    filament |= Filaments::CurrentIndex() != filament_t::NONE ? FKNOWN : 0;
    filament |= FSensors_instance().GetPrinter() == fsensor_t::NoFilament ? F_NOTSENSED : 0;

    filament_known_but_unsensed = false;
    // 4 posible states
    // 0 !FKNOWN !F_NOTSENSED
    // 1 FKNOWN !F_NOTSENSED
    // 2 !FKNOWN F_NOTSENSED
    // 3 FKNOWN F_NOTSENSED
    switch (filament) {
    case FKNOWN: //known and not "unsensed" - do not allow load
        filament_known_but_unsensed = true;
        rStateMachine.SetFsmPhase(PhasesSelftest::FirstLayer_filament_known_and_not_unsensed);
        rResult.preselect_response = Response::Next;
        break;
    case FKNOWN | F_NOTSENSED: //allow load, prepick UNLOAD, force ask preheat
        rStateMachine.SetFsmPhase(PhasesSelftest::FirstLayer_filament_not_known_or_unsensed);
        rResult.preselect_response = Response::Unload;
        break;
    case F_NOTSENSED: //allow load, prepick LOAD, force ask preheat
    case 0:           //filament is not known but is sensed == most likely same as F_NOTSENSED, but user inserted filament into sensor
    default:
        rStateMachine.SetFsmPhase(PhasesSelftest::FirstLayer_filament_not_known_or_unsensed);
        rResult.preselect_response = Response::Load;
        break;
    }
    LogInfo("%s user asked about filament");
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateAskFilament() {
    const Response response = rStateMachine.GetButtonPressed();
    switch (response) {
    case Response::Next:
        state_selected_by_user = filament_known_but_unsensed ? StateSelectedByUser::Calib : StateSelectedByUser::Preheat;
        LogInfo("%s user pressed Next", rConfig.partname);
        return LoopResult::RunNext;
    case Response::Load:
        state_selected_by_user = StateSelectedByUser::Load;
        LogInfo("%s user pressed Load", rConfig.partname);
        return LoopResult::RunNext;
    case Response::Unload:
        state_selected_by_user = StateSelectedByUser::Unload;
        LogInfo("%s user pressed Unload", rConfig.partname);
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

    queue.enqueue_one_now("M1700 W0 S"); // preheat, no return no cooldown, set filament
    LogInfo("%s preheat enqueued", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::statePreheatWaitFinished() {
    //we didn't wanted to preheat, so we are not waiting for anything
    if (state_selected_by_user != StateSelectedByUser::Preheat) {
        return LoopResult::RunNext;
    }
    //wait for operation to finish
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

    queue.enqueue_one_now("M701 W0"); // load, no return no cooldown
    LogInfo("%s load enqueued", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateFilamentLoadWaitFinished() {
    //we didn't wanted to load, so we are not waiting for anything
    if (state_selected_by_user != StateSelectedByUser::Load) {
        return LoopResult::RunNext;
    }
    //wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for load to finish", rConfig.partname);
        return LoopResult::RunCurrent;
    }
    //check if we returned from preheat or finished the load
    PreheatStatus::Result res = PreheatStatus::ConsumeResult();
    if (res == PreheatStatus::Result::DoneNoFilament) {
        // in case it flickers, we might need to add change of state
        // IPartHandler::SetFsmPhase(PhasesSelftest::);
        return LoopResult::RunNext;
    } else {
        return LoopResult::GoToMark;
    }
}

/*****************************************************************************/
// Unload
LoopResult CSelftestPart_FirstLayer::stateFilamentUnloadEnqueueGcode() {
    if (state_selected_by_user != StateSelectedByUser::Unload) {
        return LoopResult::RunNext;
    }

    queue.enqueue_one_now("M702 W0"); // unload, no return no cooldown
    LogInfo("%s unload enqueued", rConfig.partname);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateFilamentUnloadWaitFinished() {
    //we didn't wanted to unload, so we are not waiting for anything
    if (state_selected_by_user != StateSelectedByUser::Unload) {
        return LoopResult::RunNext;
    }
    //wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for unload to finish", rConfig.partname);
        return LoopResult::RunCurrent;
    }

    // in case it flickers, we might need to add change of state
    // IPartHandler::SetFsmPhase(PhasesSelftest::);

    // it does not matter if unload went well or was aborted
    // we need to ask user what to do in both cases
    return LoopResult::GoToMark;
}

LoopResult CSelftestPart_FirstLayer::stateShowCalibrateMsg() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_calib);
    return LoopResult::RunNext;
}

static constexpr int axis_steps_per_unit[] = DEFAULT_AXIS_STEPS_PER_UNIT;
static constexpr float z_offset_step = 1.0F / float(axis_steps_per_unit[2]);
static constexpr float nozzle_to_probe[3] = NOZZLE_TO_PROBE_OFFSET;
static constexpr float z_offset_def = nozzle_to_probe[2];

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
    if (current_offset_is_default)
        return LoopResult::RunNext;

    switch (rStateMachine.GetButtonPressed()) {
    case Response::No:
        probe_offset.z = z_offset_def;
        // don't return / break
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

LoopResult CSelftestPart_FirstLayer::statePrint() {
    IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_mbl);
    how_many_times_finished = FirstLayer::HowManyTimesFinished();
    queue.enqueue_one_now("G26"); // firstlay
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateMblFinished() {
    if (how_many_times_finished == FirstLayer::HowManyTimesMadeMBL())
        return LoopResult::RunCurrent;

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
    if (reprint)
        IPartHandler::SetFsmPhase(PhasesSelftest::FirstLayer_clean_sheet);
    return LoopResult::RunNext;
}

LoopResult CSelftestPart_FirstLayer::stateCleanSheet() {
    if (!reprint)
        return LoopResult::RunNext;

    switch (rStateMachine.GetButtonPressed()) {
    case Response::Next:
    case Response::Continue:
        return LoopResult::GoToMark;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult CSelftestPart_FirstLayer::stateFinish() {

    //finish
    LogInfo("%s Finished\n", rConfig.partname);
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
