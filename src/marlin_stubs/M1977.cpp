#include <marlin_stubs/M1977.hpp>

#include <client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <common/marlin_server.hpp>
#include <Marlin/src/feature/phase_stepping/calibration.hpp>
#include <Marlin/src/gcode/gcode.h>

namespace {

struct Scores {
    float p1f;
    float p1b;
    float p2f;
    float p2b;
};

enum class State {
    error,
    ok,
    nok,
    aborted,
};

struct Context {
    Scores scores_x;
    Scores scores_y;
};

constexpr bool is_ok(const Scores &scores) {
    return scores.p1f < 1.f
        && scores.p1b < 1.f
        && scores.p2f < 1.f
        && scores.p2b < 1.f;
}

fsm::PhaseData serialize_axis_nok(const Scores &scores) {
    // TODO maybe these should be saturated at 255?
    fsm::PhaseData data;
    data[0] = 100 * scores.p1f;
    data[1] = 100 * scores.p1b;
    data[2] = 100 * scores.p2f;
    data[3] = 100 * scores.p2b;
    return data;
}

fsm::PhaseData serialize_ok(const Scores &scores_x, const Scores &scores_y) {
    // take the worst of forward and backward, subtract from 1 to get reduction and scale up to percents
    fsm::PhaseData data;
    data[0] = 100 - 100 * std::max(scores_x.p1f, scores_x.p1b);
    data[1] = 100 - 100 * std::max(scores_x.p2f, scores_x.p2b);
    data[2] = 100 - 100 * std::max(scores_y.p1f, scores_y.p1b);
    data[3] = 100 - 100 * std::max(scores_y.p2f, scores_y.p2b);
    return data;
}

Response wait_for_response(const PhasesPhaseStepping phase) {
    for (;;) {
        if (Response response = marlin_server::get_response_from_phase(phase); response != Response::_none) {
            return response;
        }
        idle(true);
    }
}

class CalibrateAxisHooks final : public phase_stepping::CalibrateAxisHooks {
private:
    PhasesPhaseStepping phase;
    fsm::PhaseData data;
    int current_calibration_phase = 0;
    std::array<std::tuple<float, float>, 4> calibration_results;

public:
    Scores scores;
    State state = State::error;

    explicit CalibrateAxisHooks(PhasesPhaseStepping phase)
        : phase { phase } {
        data[0] = 0;
        data[1] = calibration_results.size();
        data[2] = 0;
    }

    void set_calibration_phases_count(int phases) override {
        if (static_cast<size_t>(phases) != calibration_results.size()) {
            bsod("phase count mismatch");
        }
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
        calibration_results[current_calibration_phase] = { forward_score, backward_score };
    };

    void on_termination() override {
        auto [p1_f, p1_b] = calibration_results[0];
        auto [p3_f, p3_b] = calibration_results[2];
        auto [p2_f, p2_b] = calibration_results[1];
        auto [p4_f, p4_b] = calibration_results[3];
        scores = Scores {
            .p1f = p1_f * p3_f,
            .p1b = p1_b * p3_b,
            .p2f = p2_f * p4_f,
            .p2b = p2_b * p4_b,
        };
        state = is_ok(scores) ? State::ok : State::nok;
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
        bsod(__FUNCTION__);
    }
}

namespace state {

    PhasesPhaseStepping intro() {
        switch (wait_for_response(PhasesPhaseStepping::intro)) {
        case Response::Continue:
            return PhasesPhaseStepping::home;
        case Response::Abort:
            // No need to invalidate test result here
            return PhasesPhaseStepping::finish;
        default:
            bsod(__FUNCTION__);
        }
    }

    PhasesPhaseStepping home() {
        marlin_server::fsm_change(PhasesPhaseStepping::home);
        GcodeSuite::G28_no_parser( // home
            true, // always_home_all
            true, // home only if needed,
            3, // raise Z by 3 mm
            false, // S-parameter,
            true, true, false // home X, Y but not Z
        );
#if HAS_TOOLCHANGER()
        tool_change(/*tool_index=*/0, tool_return_t::no_return, tool_change_lift_t::no_lift, /*z_down=*/false);
#endif
        Planner::synchronize();
        return PhasesPhaseStepping::calib_x;
    }

    PhasesPhaseStepping calib_x(Context &context) {
        CalibrateAxisHooks hooks { PhasesPhaseStepping::calib_x };
        calibration_helper(AxisEnum::X_AXIS, hooks);
        switch (hooks.state) {
        case State::error:
            return PhasesPhaseStepping::calib_error;
        case State::ok:
            context.scores_x = hooks.scores;
            return PhasesPhaseStepping::calib_y;
        case State::nok:
            return PhasesPhaseStepping::calib_x_nok;
        case State::aborted:
            return PhasesPhaseStepping::finish;
        }
        bsod(__FUNCTION__);
    }

    PhasesPhaseStepping calib_y(Context &context) {
        CalibrateAxisHooks hooks { PhasesPhaseStepping::calib_y };
        calibration_helper(AxisEnum::Y_AXIS, hooks);
        switch (hooks.state) {
        case State::error:
            return PhasesPhaseStepping::calib_error;
        case State::ok:
            context.scores_y = hooks.scores;
            return PhasesPhaseStepping::calib_ok;
        case State::nok:
            return PhasesPhaseStepping::calib_y_nok;
        case State::aborted:
            return PhasesPhaseStepping::finish;
        }
        bsod(__FUNCTION__);
    }

    PhasesPhaseStepping calib_ok(Context &context) {
        marlin_server::fsm_change(PhasesPhaseStepping::calib_ok, serialize_ok(context.scores_x, context.scores_y));
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
            bsod(__FUNCTION__);
        }
    }

    PhasesPhaseStepping calib_x_nok(Context &context) {
        marlin_server::fsm_change(PhasesPhaseStepping::calib_x_nok, serialize_axis_nok(context.scores_x));
        return fail_helper(PhasesPhaseStepping::calib_x_nok);
    }

    PhasesPhaseStepping calib_y_nok(Context &context) {
        marlin_server::fsm_change(PhasesPhaseStepping::calib_y_nok, serialize_axis_nok(context.scores_y));
        return fail_helper(PhasesPhaseStepping::calib_y_nok);
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
    case PhasesPhaseStepping::calib_x:
        return state::calib_x(context);
    case PhasesPhaseStepping::calib_y:
        return state::calib_y(context);
    case PhasesPhaseStepping::calib_x_nok:
        return state::calib_x_nok(context);
    case PhasesPhaseStepping::calib_y_nok:
        return state::calib_y_nok(context);
    case PhasesPhaseStepping::calib_error:
        return state::calib_error();
    case PhasesPhaseStepping::calib_ok:
        return state::calib_ok(context);
    case PhasesPhaseStepping::finish:
        return PhasesPhaseStepping::finish;
    }
    bsod(__FUNCTION__);
}

} // namespace

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 *  Phase Stepping Calibration Dialog. Prusa BUDDY FW specific.
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
