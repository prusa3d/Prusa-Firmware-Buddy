#include <marlin_stubs/M1977.hpp>

#include <buddy/unreachable.hpp>
#include <client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <common/marlin_server.hpp>
#include <Marlin/src/feature/phase_stepping/calibration_config.hpp>
#include <Marlin/src/feature/phase_stepping/calibration.hpp>
#include <Marlin/src/gcode/gcode.h>

namespace {

enum class State {
    error,
    finished,
    aborted,
};

static constexpr const size_t calibration_phase_count = phase_stepping::printer_calibration_config.phases.size();
struct CalibrationPhaseResult {
    float forward;
    float backward;
};
using CalibrationResult = std::array<CalibrationPhaseResult, calibration_phase_count>;

struct Context {
    CalibrationResult calibration_result_x;
    CalibrationResult calibration_result_y;
    uint8_t reduction_x;
    uint8_t reduction_y;
};

using marlin_server::wait_for_response;

class CalibrateAxisHooks final : public phase_stepping::CalibrateAxisHooks {
private:
    PhasesPhaseStepping phase;
    fsm::PhaseData data;
    int current_calibration_phase = 0;

public:
    CalibrationResult calibration_result;
    State state = State::error;

    explicit CalibrateAxisHooks(PhasesPhaseStepping phase)
        : phase { phase } {
        data[0] = 0;
        data[1] = calibration_phase_count;
        data[2] = 0;
    }

    void set_calibration_phases_count(int) override {
    }

    void on_enter_calibration_phase(int calibration_phase) override {
        data[0] = calibration_phase;
        data[2] = 0;
        marlin_server::fsm_change(phase, data);
        current_calibration_phase = calibration_phase;
    }

    void on_initial_movement() override {
    }

    void on_calibration_phase_progress(int progress) override {
        data[2] = progress;
        marlin_server::fsm_change(phase, data);
    }

    void on_calibration_phase_result(float forward_score, float backward_score) override {
        calibration_result[current_calibration_phase] = { forward_score, backward_score };
    }

    void on_termination() override {
        state = State::finished;
    }

    ContinueOrAbort on_idle() override {
        if (marlin_server::get_response_from_phase(phase) == Response::Abort) {
            state = State::aborted;
            return ContinueOrAbort::Abort;
        } else {
            return ContinueOrAbort::Continue;
        }
    }
};

void calibration_helper(AxisEnum axis, CalibrateAxisHooks &hooks) {
    auto result = phase_stepping::calibrate_axis(axis, hooks);
    if (result.has_value()) {
        phase_stepping::save_to_persistent_storage_without_enabling(axis);
    }
}

PhasesPhaseStepping fail_helper(PhasesPhaseStepping phase) {
    switch (wait_for_response(phase)) {
    case Response::Ok:
        config_store().selftest_result_phase_stepping.set(TestResult::TestResult_Failed);
        return PhasesPhaseStepping::finish;
    default:
        BUDDY_UNREACHABLE();
    }
}

static PhasesPhaseStepping intro_helper() {
#if HAS_ATTACHABLE_ACCELEROMETER()
    // Check the accelerometer now. It would be annoying to do all the homing
    // and parking moves and then tell the user to turn off the printer, just
    // to do all the moves after the reboot again.
    PrusaAccelerometer accelerometer;
    if (PrusaAccelerometer::Error::none == accelerometer.get_error()) {
        return PhasesPhaseStepping::home;
    }
    return PhasesPhaseStepping::connect_to_board;
#else
    // Proceed straight to parking stage, any accelerometer error will be reported
    // after it happens.
    return PhasesPhaseStepping::home;
#endif
}

std::optional<uint8_t> evaluate_calibration_result(const CalibrationResult &calibration_result) {
    for (const auto &res : calibration_result) {
        log_info(Marlin, "res %f %f", (double)res.backward, (double)res.forward);
    }
    constexpr const size_t halfsize = calibration_phase_count / 2;
    float reduction = 0.0f;
    for (size_t i = 0; i < halfsize; ++i) {
        const auto [coarse_forward, coarse_backward] = calibration_result[i];
        const auto [fine_forward, fine_backward] = calibration_result[i + halfsize];
        const float forward = coarse_forward * fine_forward;
        const float backward = coarse_backward * fine_backward;

        if (forward >= 1.f || backward >= 1.f) {
            return std::nullopt;
        }

        // take the worst of forward and backward, subtract from 1 to get reduction and add to accumulator
        reduction += 1.f - std::max(forward, backward);
    }
    // average + scale to percent
    return 100 * reduction / halfsize;
}

PhasesPhaseStepping evaluate_result(Context &context) {
    const auto reduction_x = evaluate_calibration_result(context.calibration_result_x);
    const auto reduction_y = evaluate_calibration_result(context.calibration_result_y);
    if (reduction_x && reduction_y) {
        context.reduction_x = *reduction_x;
        context.reduction_y = *reduction_y;
        return PhasesPhaseStepping::calib_ok;
    }
    return PhasesPhaseStepping::calib_nok;
}

namespace state {

    PhasesPhaseStepping intro() {
        switch (wait_for_response(PhasesPhaseStepping::intro)) {
        case Response::Continue:
            return intro_helper();
        case Response::Abort:
            // No need to invalidate test result here
            return PhasesPhaseStepping::finish;
        default:
            BUDDY_UNREACHABLE();
        }
    }

    // Note: This is only relevant for printers which HAS_ATTACHABLE_ACCELEROMETER()
    //       which coincidentally only have one hotend.
    constexpr uint8_t hotend = 0;
    constexpr float safe_temperature = 50;

    PhasesPhaseStepping home() {
        marlin_server::fsm_change(PhasesPhaseStepping::home);

#if HAS_ATTACHABLE_ACCELEROMETER()
        // Start cooling the hotend even before parking to save some time
        Temperature::disable_hotend();
        if (Temperature::degHotend(hotend) > safe_temperature) {
            Temperature::set_fan_speed(0, 255);
        }
#endif

        if (axes_need_homing(X_AXIS | Y_AXIS)) {
            GcodeSuite::G28_no_parser(true, true, false, { .only_if_needed = true, .z_raise = 3 }); // XY only
#if HAS_TOOLCHANGER()
            tool_change(/*tool_index=*/0, tool_return_t::no_return, tool_change_lift_t::no_lift, /*z_down=*/false);
#endif
        }
        Planner::synchronize();
#if HAS_ATTACHABLE_ACCELEROMETER()
        return PhasesPhaseStepping::wait_for_extruder_temperature;
#else
        return PhasesPhaseStepping::calib_x;
#endif
    }

#if HAS_ATTACHABLE_ACCELEROMETER()

    PhasesPhaseStepping connect_to_board(Context &) {
        marlin_server::fsm_change(PhasesPhaseStepping::connect_to_board);
        switch (wait_for_response(PhasesPhaseStepping::connect_to_board)) {
        case Response::Abort:
            return PhasesPhaseStepping::finish;
        default:
            break;
        }
        BUDDY_UNREACHABLE();
    }

    PhasesPhaseStepping wait_for_extruder_temperature(Context &) {
        for (;;) {
            switch (marlin_server::get_response_from_phase(PhasesPhaseStepping::wait_for_extruder_temperature)) {
            case Response::Abort:
                return PhasesPhaseStepping::finish;
            case Response::_none:
                if (const float temperature = Temperature::degHotend(hotend); temperature > safe_temperature) {
                    const uint16_t uint16_temperature = temperature;
                    const fsm::PhaseData data = {
                        static_cast<uint8_t>((uint16_temperature >> 8) & 0xff),
                        static_cast<uint8_t>((uint16_temperature >> 0) & 0xff),
                        0,
                        0,
                    };
                    marlin_server::fsm_change(PhasesPhaseStepping::wait_for_extruder_temperature, data);
                    idle(true);
                } else {
                    Temperature::zero_fan_speeds();
                    return PhasesPhaseStepping::attach_to_extruder;
                }
                break;
            default:
                BUDDY_UNREACHABLE();
            }
        }
        BUDDY_UNREACHABLE();
    }

    PhasesPhaseStepping attach_to_extruder(Context &) {
        marlin_server::fsm_change(PhasesPhaseStepping::attach_to_extruder);
        switch (wait_for_response(PhasesPhaseStepping::attach_to_extruder)) {
        case Response::Abort:
            return PhasesPhaseStepping::finish;
        case Response::Continue:
            return PhasesPhaseStepping::calib_x;
        default:
            break;
        }
        BUDDY_UNREACHABLE();
    }

    PhasesPhaseStepping attach_to_bed(Context &) {
        marlin_server::fsm_change(PhasesPhaseStepping::attach_to_bed);
        switch (wait_for_response(PhasesPhaseStepping::attach_to_bed)) {
        case Response::Abort:
            return PhasesPhaseStepping::finish;
        case Response::Continue:
            return PhasesPhaseStepping::calib_y;
        default:
            break;
        }
        BUDDY_UNREACHABLE();
    }

#endif

    PhasesPhaseStepping calib_x(Context &context) {
        CalibrateAxisHooks hooks { PhasesPhaseStepping::calib_x };
        calibration_helper(AxisEnum::X_AXIS, hooks);
        switch (hooks.state) {
        case State::error:
            return PhasesPhaseStepping::calib_error;
        case State::finished:
            context.calibration_result_x = hooks.calibration_result;
            return PhasesPhaseStepping::calib_y;
        case State::aborted:
            return PhasesPhaseStepping::finish;
        }
        BUDDY_UNREACHABLE();
    }

    PhasesPhaseStepping calib_y(Context &context) {
        CalibrateAxisHooks hooks { PhasesPhaseStepping::calib_y };
        calibration_helper(AxisEnum::Y_AXIS, hooks);
        switch (hooks.state) {
        case State::error:
            return PhasesPhaseStepping::calib_error;
        case State::finished:
            context.calibration_result_y = hooks.calibration_result;
            return evaluate_result(context);
        case State::aborted:
            return PhasesPhaseStepping::finish;
        }
        BUDDY_UNREACHABLE();
    }

    PhasesPhaseStepping calib_ok(Context &context) {
        fsm::PhaseData data;
        data[0] = context.reduction_x;
        data[1] = context.reduction_y;
        marlin_server::fsm_change(PhasesPhaseStepping::calib_ok, data);
        Planner::synchronize();
        phase_stepping::enable(X_AXIS, true);
        config_store().set_phase_stepping_enabled(X_AXIS, true);
        phase_stepping::enable(Y_AXIS, true);
        config_store().set_phase_stepping_enabled(Y_AXIS, true);
        config_store().selftest_result_phase_stepping.set(TestResult::TestResult_Passed);
        switch (wait_for_response(PhasesPhaseStepping::calib_ok)) {
        case Response::Ok:
            return PhasesPhaseStepping::finish;
        default:
            BUDDY_UNREACHABLE();
        }
    }

    PhasesPhaseStepping calib_nok() {
        marlin_server::fsm_change(PhasesPhaseStepping::calib_nok);
        return fail_helper(PhasesPhaseStepping::calib_nok);
    }

    PhasesPhaseStepping calib_error() {
        marlin_server::fsm_change(PhasesPhaseStepping::calib_error);
        return fail_helper(PhasesPhaseStepping::calib_error);
    }

} // namespace state

PhasesPhaseStepping get_next_phase(Context &context, const PhasesPhaseStepping phase) {
    switch (phase) {
    case PhasesPhaseStepping::intro:
        return state::intro();
    case PhasesPhaseStepping::home:
        return state::home();
#if HAS_ATTACHABLE_ACCELEROMETER()
    case PhasesPhaseStepping::connect_to_board:
        return state::connect_to_board(context);
    case PhasesPhaseStepping::wait_for_extruder_temperature:
        return state::wait_for_extruder_temperature(context);
    case PhasesPhaseStepping::attach_to_extruder:
        return state::attach_to_extruder(context);
    case PhasesPhaseStepping::attach_to_bed:
        return state::attach_to_bed(context);
#endif
    case PhasesPhaseStepping::calib_x:
        return state::calib_x(context);
    case PhasesPhaseStepping::calib_y:
        return state::calib_y(context);
    case PhasesPhaseStepping::calib_error:
        return state::calib_error();
    case PhasesPhaseStepping::calib_nok:
        return state::calib_nok();
    case PhasesPhaseStepping::calib_ok:
        return state::calib_ok(context);
    case PhasesPhaseStepping::finish:
        return PhasesPhaseStepping::finish;
    }
    BUDDY_UNREACHABLE();
}

} // namespace

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1977: Phase Stepping Calibration Dialog
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1977
 *
 */
void M1977() {
    PhasesPhaseStepping phase = PhasesPhaseStepping::intro;
    Context context {};
    marlin_server::FSM_Holder holder { phase };
    do {
        phase = get_next_phase(context, phase);
    } while (phase != PhasesPhaseStepping::finish);
}

/** @}*/

} // namespace PrusaGcodeSuite
