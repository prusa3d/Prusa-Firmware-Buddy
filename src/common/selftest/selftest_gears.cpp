#include "selftest_gears.hpp"
#include "selftest_part.hpp"

#include <M70X.hpp>
#include <Marlin/src/Marlin.h>
#include <Marlin/src/module/stepper.h>
#include <Marlin/src/module/temperature.h>
#include <common/RAII.hpp>
#include <common/filament_sensors_handler.hpp>
#include <config_store/store_instance.hpp>
#include <guiconfig/wizard_config.hpp>
#include <mapi/motion.hpp>

LOG_COMPONENT_REF(Selftest);

namespace selftest {

static SelftestGearsResult static_result; // automatically initialized by PartHandler

bool phase_gears(IPartHandler *&selftest_gears, const SelftestGearsConfig &config) {
    if (!selftest_gears) {
        selftest_gears = selftest::Factory::CreateDynamical<SelftestGears>(
            config,
            static_result,
            &SelftestGears::state_ask_first,
            &SelftestGears::state_get_fsensor_state,
            &SelftestGears::state_cycle_mark0,
            &SelftestGears::state_ask_unload_init,
            &SelftestGears::state_ask_unload_wait,
            &SelftestGears::state_filament_unload_enqueue_gcode,
            &SelftestGears::state_filament_unload_wait_finished,
            &SelftestGears::state_release_screws_init,
            &SelftestGears::state_release_screws,
            &SelftestGears::state_alignment_init,
            &SelftestGears::state_alignment,
            &SelftestGears::state_tighten_init,
            &SelftestGears::state_tighten,
            &SelftestGears::state_done_init,
            &SelftestGears::state_done);
    }

    enable_Z();
    bool in_progress = selftest_gears->Loop();

    marlin_server::fsm_change(IPartHandler::GetFsmPhase(), static_result.serialize());

    if (in_progress) {
        return true;
    }
    SelftestResult eeres = config_store().selftest_result.get();
    auto result = selftest_gears->GetResult();
    // Skipped Gears calibration are considered passed; Meant for users with prebuilt printer
    if (result == TestResult_Passed || result == TestResult_Skipped) {
        eeres.gears = TestResult_Passed;
    } else {
        eeres.gears = result;
    }

    delete selftest_gears;
    selftest_gears = nullptr;

    disable_Z();

    config_store().selftest_result.set(eeres);

    return false;
}

SelftestGears::SelftestGears(IPartHandler &state_machine, const SelftestGearsConfig &config, SelftestGearsResult &result)
    : state_machine(state_machine)
    , config(config)
    , result(result) {
}

LoopResult SelftestGears::state_ask_first() {
    const Response response = state_machine.GetButtonPressed();
    switch (response) {
    case Response::Skip:
        log_error(Selftest, "%s user pressed skip", config.partname);
        return LoopResult::Abort;
    case Response::Continue:
        need_unload = false;
        log_info(Selftest, "%s user pressed continue", config.partname);
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult SelftestGears::state_get_fsensor_state() {
    FilamentSensorState sensor_state = FilamentSensorState::Disabled;
    IFSensor *sensor = GetExtruderFSensor(0);
    if (sensor) {
        sensor_state = sensor->get_state();
    }

    switch (sensor_state) {
    case FilamentSensorState::HasFilament:
        has_filament = Filament::yes;
        break;
    case FilamentSensorState::NoFilament:
        has_filament = Filament::no;
        break;
    case FilamentSensorState::NotInitialized:
    case FilamentSensorState::NotCalibrated:
    case FilamentSensorState::NotConnected:
    case FilamentSensorState::Disabled:
        has_filament = Filament::unknown;
        break;
    }

    return LoopResult::RunNext;
}

LoopResult SelftestGears::state_ask_unload_init() {
    switch (has_filament) {
    case Filament::yes:
        IPartHandler::SetFsmPhase(PhasesSelftest::GearsCalib_filament_loaded_ask_unload);
        break;
    case Filament::no:
        return LoopResult::RunNext;
    case Filament::unknown:
        IPartHandler::SetFsmPhase(PhasesSelftest::GearsCalib_filament_unknown_ask_unload);
        break;
    }

    return LoopResult::RunNext;
}

LoopResult SelftestGears::state_ask_unload_wait() {
    if (has_filament == Filament::no) {
        return LoopResult::RunNext;
    }

    const Response response = state_machine.GetButtonPressed();
    switch (response) {
    case Response::Abort:
        log_error(Selftest, "%s user pressed abort", config.partname);
        return LoopResult::Abort;
    case Response::Unload:
        need_unload = true;
        log_info(Selftest, "%s user pressed unload", config.partname);
        return LoopResult::RunNext;
    case Response::Continue:
        need_unload = false;
        log_info(Selftest, "%s user pressed continue", config.partname);
        return LoopResult::RunNext;
    default:
        break;
    }
    return LoopResult::RunCurrent;
}

LoopResult SelftestGears::state_filament_unload_enqueue_gcode() {
    if (need_unload) {
        queue.enqueue_one_now("M702 W2"); // unload with return option
        log_info(Selftest, "%s unload enqueued", config.partname);
    }
    return LoopResult::RunNext;
}

LoopResult SelftestGears::state_filament_unload_wait_finished() {
    // we didn't wanted to unload, so we are not waiting for anything
    if (!need_unload) {
        return LoopResult::RunNext;
    }
    // wait for operation to finish
    if (filament_gcodes::InProgress::Active()) {
        LogInfoTimed(log, "%s waiting for unload to finish", config.partname);
        return LoopResult::RunCurrent;
    }
    // check if we returned from preheat or finished the unload
    PreheatStatus::Result res = PreheatStatus::ConsumeResult();
    if (res == PreheatStatus::Result::DoneNoFilament) {
        queue.enqueue_one_now("M104 S0"); // cool down the nozzle
        queue.enqueue_one_now("M140 S0"); // cool down the heatbed
        return LoopResult::RunNext;
    }
    return LoopResult::GoToMark0;
}

LoopResult SelftestGears::state_release_screws_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::GearsCalib_release_screws);
    return LoopResult::RunNext;
}

LoopResult SelftestGears::state_release_screws() {
    const Response response = state_machine.GetButtonPressed();

    switch (response) {
    case Response::Continue:
        return LoopResult::RunNext;
    case Response::Skip:
        return LoopResult::Abort;
    default:
        return LoopResult::RunCurrent;
    }
}

LoopResult SelftestGears::state_alignment_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::GearsCalib_alignment);
    return LoopResult::RunNext;
}

LoopResult SelftestGears::state_alignment() {
    move_gear();

    if (state_machine.IsInState_ms() > 20'000) {
        return LoopResult::RunNext;
    } else {
        return LoopResult::RunCurrent;
    }
}

LoopResult SelftestGears::state_tighten_init() {
    // prevent stop of turning gears at phase switch
    move_gear();

    IPartHandler::SetFsmPhase(PhasesSelftest::GearsCalib_tighten);
    return LoopResult::RunNext;
}

LoopResult SelftestGears::state_tighten() {
    move_gear();

    const Response response = state_machine.GetButtonPressed();

    switch (response) {
    case Response::Continue:
        return LoopResult::RunNext;
    default:
        return LoopResult::RunCurrent;
    }
}

LoopResult SelftestGears::state_done_init() {
    IPartHandler::SetFsmPhase(PhasesSelftest::GearsCalib_done);
    return LoopResult::RunNext;
}

LoopResult SelftestGears::state_done() {
    const Response response = state_machine.GetButtonPressed();

    switch (response) {
    case Response::Continue:
        return LoopResult::RunNext;
    default:
        return LoopResult::RunCurrent;
    }
}

void SelftestGears::move_gear() {
    AutoRestore<bool> CE(thermalManager.allow_cold_extrude);
    thermalManager.allow_cold_extrude = true;
    mapi::extruder_schedule_turning(config.feedrate, -0.6);
}

} // namespace selftest
