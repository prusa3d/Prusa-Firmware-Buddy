#include <common/selftest_phase_stepping_result.hpp>

namespace {

void serialize_axis_nok(fsm::PhaseData &data, const phase_stepping::CalibrationResult &result) {
    const phase_stepping::CalibrationResult::Scores scores = result.get_scores();
    // TODO maybe these should be saturated at 255?
    data[0] = 100 * scores.p1f;
    data[1] = 100 * scores.p1b;
    data[2] = 100 * scores.p2f;
    data[3] = 100 * scores.p2b;
}

void serialize_ok(fsm::PhaseData &data, const phase_stepping::CalibrationResult &result_x, const phase_stepping::CalibrationResult &result_y) {
    const phase_stepping::CalibrationResult::Scores scores_x = result_x.get_scores();
    const phase_stepping::CalibrationResult::Scores scores_y = result_y.get_scores();
    // take the worst of forward and backward, subtract from 1 to get reduction and scale up to percents
    data[0] = 100 - 100 * std::max(scores_x.p1f, scores_x.p1b);
    data[1] = 100 - 100 * std::max(scores_x.p2f, scores_x.p2b);
    data[2] = 100 - 100 * std::max(scores_y.p1f, scores_y.p1b);
    data[3] = 100 - 100 * std::max(scores_y.p2f, scores_y.p2b);
}

} // namespace

fsm::PhaseData SelftestPhaseSteppingResult::serialize(PhasesSelftest phase) const {
    fsm::PhaseData data;
    switch (phase) {
    case PhasesSelftest::PhaseStepping_pick_tool:
    case PhasesSelftest::PhaseStepping_calib_x:
    case PhasesSelftest::PhaseStepping_calib_y:
    case PhasesSelftest::PhaseStepping_calib_error:
    case PhasesSelftest::PhaseStepping_enabling:
        break;
    case PhasesSelftest::PhaseStepping_calib_ok:
        serialize_ok(data, result_x, result_y);
        break;
    case PhasesSelftest::PhaseStepping_calib_x_nok:
        serialize_axis_nok(data, result_x);
        break;
    case PhasesSelftest::PhaseStepping_calib_y_nok:
        serialize_axis_nok(data, result_y);
        break;
    default:
        abort();
    }
    return data;
}
