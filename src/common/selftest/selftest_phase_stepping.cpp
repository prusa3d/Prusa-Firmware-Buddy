#include "selftest_phase_stepping.hpp"
#include <guiconfig/wizard_config.hpp>
#include "i_selftest.hpp"

#include <option/has_toolchanger.h>
#include <feature/phase_stepping/calibration.hpp>

LOG_COMPONENT_REF(Selftest);

namespace selftest {

static SelftestPhaseSteppingResult static_result; // automatically initialized by PartHandler

namespace {

    constexpr bool is_ok(const phase_stepping::CalibrationResult::Scores &scores) {
        return scores.p1f < 1.f
            && scores.p1b < 1.f
            && scores.p2f < 1.f
            && scores.p2b < 1.f;
    }

} // namespace

bool phase_phase_stepping(IPartHandler *&selftest_phase_stepping, const SelftestPhaseSteppingConfig &config) {
    if (!selftest_phase_stepping) {
        selftest_phase_stepping = selftest::Factory::CreateDynamical<SelftestPhaseStepping>(
            config,
            static_result,
            &SelftestPhaseStepping::state_intro,
            &SelftestPhaseStepping::state_calib_pick_tool,
            &SelftestPhaseStepping::state_wait_until_done,
            &SelftestPhaseStepping::state_calib_x,
            &SelftestPhaseStepping::state_wait_until_done,
            &SelftestPhaseStepping::state_calib_y,
            &SelftestPhaseStepping::state_wait_until_done,
            &SelftestPhaseStepping::state_calib_enable,
            &SelftestPhaseStepping::state_calib_save,
            &SelftestPhaseStepping::state_wait_until_done);
    }

    bool in_progress = selftest_phase_stepping->Loop();

    const PhasesSelftest phase = IPartHandler::GetFsmPhase();
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, phase, static_result.serialize(phase));

    if (in_progress) {
        return true;
    }

    config_store().selftest_result_phase_stepping.set(selftest_phase_stepping->GetResult());

    delete selftest_phase_stepping;
    selftest_phase_stepping = nullptr;

    return false;
}

SelftestPhaseStepping::SelftestPhaseStepping(IPartHandler &state_machine, const SelftestPhaseSteppingConfig &config, SelftestPhaseSteppingResult &result)
    : state_machine(state_machine)
    , config(config)
    , result(result) {
}

LoopResult SelftestPhaseStepping::state_intro() {
    IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_intro);
    switch (state_machine.GetButtonPressed()) {
    case Response::Abort:
        return LoopResult::Abort;
    case Response::Continue:
        return LoopResult::RunNext;
    default:
        return LoopResult::RunCurrent;
    }
}

LoopResult SelftestPhaseStepping::state_calib_pick_tool() {
#if HAS_TOOLCHANGER()
    IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_pick_tool);
    marlin_server::enqueue_gcode("T0 S1 L0 D0"); // pick zeroth
#endif
    return LoopResult::RunNext;
}

LoopResult SelftestPhaseStepping::state_calib_x() {
    IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_calib_x);
    marlin_server::enqueue_gcode("M977 X");
    return LoopResult::RunNext;
}

LoopResult SelftestPhaseStepping::state_calib_y() {
    static_result.result_x = phase_stepping::last_calibration_result;
    switch (static_result.result_x.get_state()) {
    case phase_stepping::CalibrationResult::State::unknown:
        log_error(Selftest, "x calibration result unknown");
        [[fallthrough]];
    case phase_stepping::CalibrationResult::State::error:
        return handle_button(PhasesSelftest::PhaseStepping_calib_error, LoopResult::Abort);
    case phase_stepping::CalibrationResult::State::known:
        if (is_ok(static_result.result_x.get_scores())) {
            IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_calib_y);
            marlin_server::enqueue_gcode("M977 Y");
            return LoopResult::RunNext;
        } else {
            return handle_button(PhasesSelftest::PhaseStepping_calib_x_nok, LoopResult::Abort);
        }
    }
    abort();
}

LoopResult SelftestPhaseStepping::state_calib_enable() {
    static_result.result_y = phase_stepping::last_calibration_result;
    switch (static_result.result_y.get_state()) {
    case phase_stepping::CalibrationResult::State::unknown:
        log_error(Selftest, "y calibration result unknown");
        [[fallthrough]];
    case phase_stepping::CalibrationResult::State::error:
        return handle_button(PhasesSelftest::PhaseStepping_calib_error, LoopResult::Abort);
    case phase_stepping::CalibrationResult::State::known:
        if (is_ok(static_result.result_y.get_scores())) {
            return handle_button(PhasesSelftest::PhaseStepping_calib_ok, LoopResult::RunNext);
        } else {
            return handle_button(PhasesSelftest::PhaseStepping_calib_y_nok, LoopResult::Abort);
        }
    }
    abort();
}

LoopResult SelftestPhaseStepping::state_calib_save() {
    IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_enabling);
    marlin_server::enqueue_gcode("M970 X Y");
    return LoopResult::RunNext;
}

LoopResult SelftestPhaseStepping::handle_button(PhasesSelftest phase, LoopResult next) {
    IPartHandler::SetFsmPhase(phase);
    if (state_machine.GetButtonPressed() != Response::Ok) {
        return LoopResult::RunCurrent;
    }
    return next;
}

LoopResult SelftestPhaseStepping::state_wait_until_done() {
    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

} // namespace selftest
