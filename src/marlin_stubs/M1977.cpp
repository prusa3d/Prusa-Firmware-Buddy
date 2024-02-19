#include <marlin_stubs/M1977.hpp>

#include <common/client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <common/marlin_server.hpp>
#include <Marlin/src/feature/phase_stepping/calibration.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <option/has_toolchanger.h>

namespace {

struct Context {
    phase_stepping::CalibrationResult result_x { phase_stepping::CalibrationResult::make_unknown() };
    phase_stepping::CalibrationResult result_y { phase_stepping::CalibrationResult::make_unknown() };

    fsm::PhaseData serialize(PhasesPhaseStepping) const;
};

constexpr bool is_ok(const phase_stepping::CalibrationResult::Scores &scores) {
    return scores.p1f < 1.f
        && scores.p1b < 1.f
        && scores.p2f < 1.f
        && scores.p2b < 1.f;
}

Response wait_for_response(const PhasesPhaseStepping phase) {
    for (;;) {
        if (Response response = marlin_server::get_response_from_phase(phase); response != Response::_none) {
            return response;
        }
        idle(true);
    }
}

class CalibrationReporter : public phase_stepping::CalibrationReporterBase {
public:
    std::array<std::tuple<float, float>, 4> _calibration_results;
    phase_stepping::CalibrationResult result = phase_stepping::CalibrationResult::make_error();

    void set_calibration_phases_count(int phases) override {
        if (static_cast<size_t>(phases) != _calibration_results.size()) {
            bsod("phase count mismatch");
        }
        phase_stepping::CalibrationReporterBase::set_calibration_phases_count(phases);
    }

    void on_initial_movement() override {
    }

    void on_calibration_phase_progress(int) override {
        // TODO report calibration progress
    }

    void on_calibration_phase_result(float forward_score, float backward_score) override {
        _calibration_results[_current_calibration_phase] = { forward_score, backward_score };
    };

    void on_termination() override {
        auto [p1_f, p1_b] = _calibration_results[0];
        auto [p3_f, p3_b] = _calibration_results[2];
        auto [p2_f, p2_b] = _calibration_results[1];
        auto [p4_f, p4_b] = _calibration_results[3];
        result = phase_stepping::CalibrationResult::make_known(
            phase_stepping::CalibrationResult::Scores {
                .p1f = p1_f * p3_f,
                .p1b = p1_b * p3_b,
                .p2f = p2_f * p4_f,
                .p2b = p2_b * p4_b,
            });
    }
};

void calibration_helper(AxisEnum axis, CalibrationReporter &calibration_reporter) {
    auto result = phase_stepping::calibrate_axis(axis, calibration_reporter);
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
            return PhasesPhaseStepping::pick_tool;
        case Response::Abort:
            // No need to invalidate test result here
            return PhasesPhaseStepping::finish;
        default:
            bsod(__FUNCTION__);
        }
    }

    PhasesPhaseStepping pick_tool() {
        GcodeSuite::G28_no_parser( // home
            true, // always_home_all
            true, // home only if needed,
            3, // raise Z by 3 mm
            false, // S-parameter,
            true, true, false // home X, Y but not Z
        );
        tool_change(/*tool_index=*/0, tool_return_t::no_return, tool_change_lift_t::no_lift, /*z_down=*/false);
        Planner::synchronize();
        return PhasesPhaseStepping::calib_x;
    }

    PhasesPhaseStepping calib_x(Context &context) {
        CalibrationReporter calibration_reporter;
        calibration_helper(AxisEnum::X_AXIS, calibration_reporter);
        switch (calibration_reporter.result.get_state()) {
        case phase_stepping::CalibrationResult::State::unknown:
            [[fallthrough]];
        case phase_stepping::CalibrationResult::State::error:
            return PhasesPhaseStepping::calib_error;
        case phase_stepping::CalibrationResult::State::known:
            if (is_ok(calibration_reporter.result.get_scores())) {
                context.result_x = calibration_reporter.result;
                return PhasesPhaseStepping::calib_y;
            } else {
                return PhasesPhaseStepping::calib_x_nok;
            }
        }
        bsod(__FUNCTION__);
    }

    PhasesPhaseStepping calib_y(Context &context) {
        CalibrationReporter calibration_reporter;
        calibration_helper(AxisEnum::Y_AXIS, calibration_reporter);
        switch (calibration_reporter.result.get_state()) {
        case phase_stepping::CalibrationResult::State::unknown:
            [[fallthrough]];
        case phase_stepping::CalibrationResult::State::error:
            return PhasesPhaseStepping::calib_error;
        case phase_stepping::CalibrationResult::State::known:
            if (is_ok(calibration_reporter.result.get_scores())) {
                context.result_y = calibration_reporter.result;
                return PhasesPhaseStepping::calib_ok;
            } else {
                return PhasesPhaseStepping::calib_y_nok;
            }
        }
        bsod(__FUNCTION__);
    }

    PhasesPhaseStepping calib_ok() {
        switch (wait_for_response(PhasesPhaseStepping::calib_ok)) {
        case Response::Ok:
            return PhasesPhaseStepping::enabling;
        default:
            bsod(__FUNCTION__);
        }
    }

    PhasesPhaseStepping enabling() {
        Planner::synchronize();
        phase_stepping::enable(X_AXIS, true);
        config_store().set_phase_stepping_enabled(X_AXIS, true);
        phase_stepping::enable(Y_AXIS, true);
        config_store().set_phase_stepping_enabled(Y_AXIS, true);
        config_store().selftest_result_phase_stepping.set(TestResult::TestResult_Passed);
        return PhasesPhaseStepping::finish;
    }

    PhasesPhaseStepping calib_x_nok() {
        return fail_helper(PhasesPhaseStepping::calib_x_nok);
    }

    PhasesPhaseStepping calib_y_nok() {
        return fail_helper(PhasesPhaseStepping::calib_y_nok);
    }

    PhasesPhaseStepping calib_error() {
        return fail_helper(PhasesPhaseStepping::calib_error);
    }

} // namespace state

fsm::PhaseData serialize_axis_nok(const phase_stepping::CalibrationResult &result) {
    const phase_stepping::CalibrationResult::Scores scores = result.get_scores();
    // TODO maybe these should be saturated at 255?
    fsm::PhaseData data;
    data[0] = 100 * scores.p1f;
    data[1] = 100 * scores.p1b;
    data[2] = 100 * scores.p2f;
    data[3] = 100 * scores.p2b;
    return data;
}

fsm::PhaseData serialize_ok(const phase_stepping::CalibrationResult &result_x, const phase_stepping::CalibrationResult &result_y) {
    const phase_stepping::CalibrationResult::Scores scores_x = result_x.get_scores();
    const phase_stepping::CalibrationResult::Scores scores_y = result_y.get_scores();
    // take the worst of forward and backward, subtract from 1 to get reduction and scale up to percents
    fsm::PhaseData data;
    data[0] = 100 - 100 * std::max(scores_x.p1f, scores_x.p1b);
    data[1] = 100 - 100 * std::max(scores_x.p2f, scores_x.p2b);
    data[2] = 100 - 100 * std::max(scores_y.p1f, scores_y.p1b);
    data[3] = 100 - 100 * std::max(scores_y.p2f, scores_y.p2b);
    return data;
}

fsm::PhaseData Context::serialize(PhasesPhaseStepping phase) const {
    switch (phase) {
    case PhasesPhaseStepping::intro:
    case PhasesPhaseStepping::pick_tool:
    case PhasesPhaseStepping::calib_x:
    case PhasesPhaseStepping::calib_y:
    case PhasesPhaseStepping::calib_error:
    case PhasesPhaseStepping::enabling:
    case PhasesPhaseStepping::finish:
        return fsm::PhaseData {};
    case PhasesPhaseStepping::calib_ok:
        return serialize_ok(result_x, result_y);
    case PhasesPhaseStepping::calib_x_nok:
        return serialize_axis_nok(result_x);
    case PhasesPhaseStepping::calib_y_nok:
        return serialize_axis_nok(result_y);
    }
    bsod(__FUNCTION__);
}

PhasesPhaseStepping get_next_phase(Context &context, const PhasesPhaseStepping phase) {
    switch (phase) {
    case PhasesPhaseStepping::intro:
        return state::intro();
    case PhasesPhaseStepping::pick_tool:
        return state::pick_tool();
    case PhasesPhaseStepping::calib_x:
        return state::calib_x(context);
    case PhasesPhaseStepping::calib_y:
        return state::calib_y(context);
    case PhasesPhaseStepping::calib_x_nok:
        return state::calib_x_nok();
    case PhasesPhaseStepping::calib_y_nok:
        return state::calib_y_nok();
    case PhasesPhaseStepping::calib_error:
        return state::calib_error();
    case PhasesPhaseStepping::calib_ok:
        return state::calib_ok();
    case PhasesPhaseStepping::enabling:
        return state::enabling();
    case PhasesPhaseStepping::finish:
        return PhasesPhaseStepping::finish;
    }
    bsod(__FUNCTION__);
}

} // namespace

namespace PrusaGcodeSuite {

void M1977() {
    PhasesPhaseStepping phase = PhasesPhaseStepping::intro;
    Context context;
    FSM_HOLDER_WITH_DATA__LOGGING(PhaseStepping, phase, {});
    do {
        phase = get_next_phase(context, phase);
        FSM_CHANGE_WITH_DATA__LOGGING(phase, context.serialize(phase));
    } while (phase != PhasesPhaseStepping::finish);
}

} // namespace PrusaGcodeSuite
