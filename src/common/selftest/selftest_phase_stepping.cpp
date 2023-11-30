#include "selftest_phase_stepping.hpp"
#include "wizard_config.hpp"
#include "i_selftest.hpp"

#include <option/has_toolchanger.h>
#include <feature/phase_stepping/calibration.hpp>

LOG_COMPONENT_REF(Selftest);

namespace selftest {

static SelftestPhaseSteppingResult static_result; // automatically initialized by PartHandler

bool phase_phase_stepping(IPartHandler *&selftest_phase_stepping, const SelftestPhaseSteppingConfig &config) {
    if (!selftest_phase_stepping) {
        selftest_phase_stepping = selftest::Factory::CreateDynamical<SelftestPhaseStepping>(
            config,
            static_result,
            &SelftestPhaseStepping::state_calib_pick_tool,
            &SelftestPhaseStepping::state_wait_until_done,
            &SelftestPhaseStepping::state_calib_x,
            &SelftestPhaseStepping::state_wait_until_done,
            &SelftestPhaseStepping::state_calib_y,
            &SelftestPhaseStepping::state_wait_until_done,
            &SelftestPhaseStepping::state_calib_enable,
            &SelftestPhaseStepping::state_wait_until_done);
    }

    bool in_progress = selftest_phase_stepping->Loop();

    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), static_result.serialize());

    if (in_progress) {
        return true;
    }
    SelftestResult eeres = config_store().selftest_result.get();
    eeres.phase_stepping = selftest_phase_stepping->GetResult();

    delete selftest_phase_stepping;
    selftest_phase_stepping = nullptr;

    config_store().selftest_result.set(eeres);

    return false;
}

SelftestPhaseStepping::SelftestPhaseStepping(IPartHandler &state_machine, const SelftestPhaseSteppingConfig &config, SelftestPhaseSteppingResult &result)
    : state_machine(state_machine)
    , config(config)
    , result(result) {
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
    // check for calib X error
    if (phase_stepping::last_calibration_result != phase_stepping::CalibrationResult::Ok) {
        return handle_error();
    }
    IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_calib_y);
    marlin_server::enqueue_gcode("M977 Y");
    return LoopResult::RunNext;
}

LoopResult SelftestPhaseStepping::state_calib_enable() {
    // check for calib Y error
    if (phase_stepping::last_calibration_result != phase_stepping::CalibrationResult::Ok) {
        return handle_error();
    }
    IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_enabling);
    marlin_server::enqueue_gcode("M970 X Y");
    return LoopResult::RunNext;
}

LoopResult SelftestPhaseStepping::handle_error() {
    IPartHandler::SetFsmPhase(PhasesSelftest::PhaseStepping_calib_failed);
    if (state_machine.GetButtonPressed() != Response::Ok) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::Abort;
}

LoopResult SelftestPhaseStepping::state_wait_until_done() {
    if (queue.has_commands_queued() || planner.processing()) {
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

} // namespace selftest
