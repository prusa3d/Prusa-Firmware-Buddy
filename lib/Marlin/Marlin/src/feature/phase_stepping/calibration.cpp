#include "calibration.hpp"
#include "phase_stepping.hpp"
#include "calibration_config.hpp"

#include <logging/log.hpp>
#include <module/planner.h>
#include <module/motion.h>
#include <gcode/gcode.h>

#include <vector>
#include <cmath>
#include <numbers>

using namespace phase_stepping;
using namespace phase_stepping::opts;

LOG_COMPONENT_REF(PhaseStepping);

// Temporary debugging to Marlin serial for convenience
#define SERIAL_DEBUG

// Poor man's "coroutine-like" golden search algorithm. It is instantiated with
// initial range. The method run() runs the algorithm until it needs to evaluate
// the cost function. You can get the requested point of evaluation via x(), and
// you submit the result via submit(x, f(x)). Then you have to again, run it via
// run() and so on until you reach desired precision.
class InterruptableGoldenSearch {
    static constexpr float PHI = std::numbers::phi_v<float>;

    enum class Phase {
        NotStarted = 0,
        InitialFcRequested = 1,
        InitialFdRequested = 2,
        FcRequested = 3,
        FdRequested = 4,
    };

    Phase _phase = Phase::NotStarted;
    int _iterations = 0, _evals = 0;
    float _a, _b, _c, _d;
    float _fc = 0, _fd = 0;
    float _requested_x = 0, _result = 0;

    const char *_debug_name;

    void _do_iteration() {
        log_info(PhaseStepping, "%s: it %d, bracket (%f, %f) = (%f, %f)",
            _debug_name, _iterations, _c, _d, _fc, _fd);

#ifdef SERIAL_DEBUG
        SERIAL_ECHO(_debug_name);
        SERIAL_ECHO(": it ");
        SERIAL_ECHO(_iterations);
        SERIAL_ECHO(", bracket (");
        SERIAL_PRINT(_c, SERIAL_DECIMALS);
        SERIAL_ECHO(", ");
        SERIAL_PRINT(_d, SERIAL_DECIMALS);
        SERIAL_ECHO(") = (");
        SERIAL_PRINT(_fc, SERIAL_DECIMALS);
        SERIAL_ECHO(", ");
        SERIAL_PRINT(_fd, SERIAL_DECIMALS);
        SERIAL_ECHOLN(")");
#endif

        if (_fc < _fd) {
            _b = _d;
            _d = _c;
            _fd = _fc;
            _c = _b - (_b - _a) / PHI;
            _requested_x = _c;
            _phase = Phase::FcRequested;
        } else {
            _a = _c;
            _c = _d;
            _fc = _fd;
            _d = _a + (_b - _a) / PHI;
            _requested_x = _d;
            _phase = Phase::FdRequested;
        }
        _iterations++;
    }

public:
    InterruptableGoldenSearch(float a, float b, const char *name = "unnamed")
        : _a(a)
        , _b(b)
        , _debug_name(name) {}

    float x() const {
        return _requested_x;
    }

    void submit(float fx) {
        _result = fx;
    }

    void run() {
        switch (_phase) {
        case Phase::NotStarted:
            _c = _b - (_b - _a) / PHI;
            _d = _a + (_b - _a) / PHI;
            _requested_x = _c;
            _phase = Phase::InitialFcRequested;
            break;
        case Phase::InitialFcRequested:
            _fc = _result;
            _requested_x = _d;
            _phase = Phase::InitialFdRequested;
            break;
        case Phase::InitialFdRequested:
            _fd = _result;

#ifdef SERIAL_DEBUG
            SERIAL_ECHO(_debug_name);
            SERIAL_ECHO(": ");
            SERIAL_ECHO("Initial bracket (");
            SERIAL_PRINT(_c, SERIAL_DECIMALS);
            SERIAL_ECHO(", ");
            SERIAL_PRINT(_d, SERIAL_DECIMALS);
            SERIAL_ECHO(") = (");
            SERIAL_PRINT(_fc, SERIAL_DECIMALS);
            SERIAL_ECHO(", ");
            SERIAL_PRINT(_fd, SERIAL_DECIMALS);
            SERIAL_ECHOLN(")");
#endif

            _do_iteration();
            break;
        case Phase::FcRequested:
            _fc = _result;
            _do_iteration();
            break;
        case Phase::FdRequested:
            _fd = _result;
            _do_iteration();
        }
        _evals++;
    }

    float step_size() const {
        return std::abs(_b - _a);
    }

    int iterations() const {
        return _iterations;
    }

    int evals() const {
        return _evals;
    }

    float arg_min() const {
        return _fc < _fd ? _c : _d;
    }

    float min() const {
        return std::min(_fc, _fd);
    }
};

// Given change in physical space, return change in logical axis.
// - for cartesian this is identity,
// - for CORE XY it converts (A, B) to (X, Y)
static std::tuple<float, float> physical_to_logical(float x, float y) {
#ifdef COREXY
    return {
        (x + y) / 2,
        (x - y) / 2
    };
#else
    return { x, y };
#endif
}

// @brief Wait for a state of a given axis specified by a predicate.
// @return true on success, false on timeout
template <typename Pred>
static bool wait_for_movement_state(phase_stepping::AxisState &axis_state,
    int timeout_ms, Pred pred) {
    auto start_time = ticks_ms();

    while (!(pred(axis_state))) {
        auto cur_time = ticks_ms();
        if (ticks_diff(cur_time, start_time) > timeout_ms) {
            return false;
        } else {
            idle(true, true);
        }
    }
    return true;
}

// Wait for end of acceleration phase of a given axis. Returns true on success,
// false on timeout
static bool wait_for_accel_end(phase_stepping::AxisState &axis_state,
    int timeout_ms = 1000) {
    return wait_for_movement_state(axis_state, timeout_ms, [](phase_stepping::AxisState &s) {
        return s.is_cruising.load();
    });
}

#if !PRINTER_IS_PRUSA_COREONE() && !PRINTER_IS_PRUSA_MK4()
// Computes a pseudo-projection of one vector to another. The length of
// direction vector is not normalized.
static float pseudo_project(std::tuple<float, float> what, std::tuple<float, float> dir) {
    return std::get<0>(what) * std::get<0>(dir) + std::get<1>(what) * std::get<1>(dir);
}
#endif

static float project_to_axis(AxisEnum axis, const PrusaAccelerometer::Acceleration &sample) {
#if PRINTER_IS_PRUSA_COREONE()
    // TODO do this properly. Somehow.
    if (axis == AxisEnum::X_AXIS) {
        return sample.val[1];
    } else if (axis == AxisEnum::Y_AXIS) {
        return sample.val[0];
    } else {
        bsod("Unsupported axis");
    }
#elif ENABLED(COREXY)
    std::pair<float, float> proj_dir;
    if (axis == AxisEnum::X_AXIS) {
        proj_dir = { M_SQRT1_2, M_SQRT1_2 };
    } else if (axis == AxisEnum::Y_AXIS) {
        proj_dir = { -M_SQRT1_2, M_SQRT1_2 };
    } else {
        bsod("Unsupported axis");
    }

    return pseudo_project({ sample.val[0], sample.val[1] }, proj_dir);
#else
    if (axis == AxisEnum::X_AXIS) {
        return sample.val[0];
    } else if (axis == AxisEnum::Y_AXIS) {
        return sample.val[1];
    } else {
        bsod("Unsupported axis");
    }
#endif
}

// Naively compute n-th bin of DFT and return its amplitude
static float dft_n_mag(const std::vector<float> &signal, int n) {
    assert(n >= 0);

    float sum_sin = 0, sum_cos = 0;
    for (std::size_t i = 0; i != signal.size(); i++) {
        float x = 2 * std::numbers::pi_v<float> * i * n / signal.size();
        float y = signal[i];
        sum_sin += y * sin(x);
        sum_cos += y * cos(x);
    }

    return std::sqrt(sum_sin * sum_sin + sum_cos * sum_cos);
}

// Given desired revolutions and speed, return expected revolutions for
// measurements and required delay before measurement in seconds
static std::tuple<float, float> sample_capture_revs(float revs, float speed) {
    static const float MOVE_MARGIN = 0.2; // To account for acceleration and deceleration we use simple hard-coded constant, revs.
    static const float ACC_RESONANCE_PERIOD = 0.2; // To account for print head resonances after acceleration, seconds.

    return {
        fabs(revs) + MOVE_MARGIN + ACC_RESONANCE_PERIOD * speed,
        ACC_RESONANCE_PERIOD
    };
}

static void move_to_calibration_start(AxisEnum axis, const CalibrationPhase &phase_config) {
    auto [measurement_revs, _] = sample_capture_revs(phase_config.revs, phase_config.speed);

    if (axis_states[axis].inverted) {
        measurement_revs = -measurement_revs;
    }

    float a_revs = axis == AxisEnum::A_AXIS ? measurement_revs : 0;
    float b_revs = axis == AxisEnum::B_AXIS ? measurement_revs : 0;
    const auto [d_rot_x, d_rot_y] = physical_to_logical(a_revs, b_revs);
    const float dx = rev_to_mm(AxisEnum::X_AXIS, d_rot_x);
    const float dy = rev_to_mm(AxisEnum::Y_AXIS, d_rot_y);

    const float target_x = X_BED_SIZE / 2.f - dx / 2.f;
    const float target_y = Y_BED_SIZE / 2.f - dy / 2.f;

    do_blocking_move_to_xy(target_x, target_y);
}

static float plan_no_movement_block(AxisEnum physical_axis, int direction, float duration) {
    assert(direction == 1 || direction == -1);
    assert(duration >= 0);

    // We fake no movement synchronous with planner via arbitrarily small speed.
    // We accelerate to this speed and then decelerate to zero.
    static const float NO_MOVEMENT_SPEED = 0.1; // mm/s
    float accel = 2 * NO_MOVEMENT_SPEED / duration;
    float dist = direction * 2 * (0.5 * accel * duration * duration / 4); // Up and down

    auto [d_x, d_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? dist : 0,
        physical_axis == AxisEnum::Y_AXIS ? dist : 0);
    auto [speed_x, speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? NO_MOVEMENT_SPEED : 0,
        physical_axis == AxisEnum::Y_AXIS ? NO_MOVEMENT_SPEED : 0);
    auto [accel_x, accel_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? accel : 0,
        physical_axis == AxisEnum::Y_AXIS ? accel : 0);

    float block_speed = sqrt(speed_x * speed_x + speed_y * speed_y);
    float block_accel = sqrt(accel_x * accel_x + accel_y * accel_y);

    auto target = current_position;
    target.x += d_x;
    target.y += d_y;

    log_debug(PhaseStepping, "No movement block: dist (%f, %f), speed (%f, %f), accel (%f, %f)",
        d_x, d_y, speed_x, speed_y, accel_x, accel_y);

    Planner::buffer_raw_line(target, block_accel, block_speed, 0, 0, active_extruder);
    current_position = target;

    return duration;
}

static float plan_constant_movement_block(AxisEnum physical_axis, int direction, float speed, float revs) {
    assert(physical_axis == AxisEnum::X_AXIS || physical_axis == AxisEnum::Y_AXIS);
    assert(direction == 1 || direction == -1);
    assert(speed >= 0);
    assert(revs > 0);

    static const float DUMMY_ACCEL = 1;

    float duration = revs / speed;

    auto [x_revs, y_revs] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? direction * revs : 0,
        physical_axis == AxisEnum::Y_AXIS ? direction * revs : 0);
    auto [speed_x, speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? speed : 0);

    float speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, speed_x);
    float speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, speed_y);
    float block_speed = sqrt(speed_x_mm * speed_x_mm + speed_y_mm * speed_y_mm);

    auto target = current_position;
    target.x += rev_to_mm(AxisEnum::X_AXIS, x_revs);
    target.y += rev_to_mm(AxisEnum::Y_AXIS, y_revs);

    Planner::buffer_raw_line(target, DUMMY_ACCEL, block_speed, block_speed, block_speed, active_extruder);
    current_position = target;

    return duration;
}

static float plan_accel_block(AxisEnum physical_axis, int direction, float start_speed, float end_speed, float accel = 100) {
    assert(physical_axis == AxisEnum::X_AXIS || physical_axis == AxisEnum::Y_AXIS);
    assert(direction == 1 || direction == -1);
    assert(start_speed >= 0 && end_speed >= 0);
    assert(accel > 0);

    if (start_speed == end_speed) {
        return 0;
    }

    float duration = std::fabs((end_speed - start_speed)) / accel;
    float revs = direction * 0.5f * accel * duration * duration;

    auto [revs_x, revs_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? revs : 0,
        physical_axis == AxisEnum::Y_AXIS ? revs : 0);
    auto [start_speed_x, start_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? start_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? start_speed : 0);
    auto [end_speed_x, end_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? end_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? end_speed : 0);
    auto [accel_x, accel_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? accel : 0,
        physical_axis == AxisEnum::Y_AXIS ? accel : 0);

    float start_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, start_speed_x);
    float start_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, start_speed_y);
    float block_start_speed = sqrt(start_speed_x_mm * start_speed_x_mm + start_speed_y_mm * start_speed_y_mm);

    float end_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, end_speed_x);
    float end_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, end_speed_y);
    float block_end_speed = sqrt(end_speed_x_mm * end_speed_x_mm + end_speed_y_mm * end_speed_y_mm);

    float block_nominal_speed = std::max(block_start_speed, block_end_speed);

    float accel_x_mm = rev_to_mm(AxisEnum::X_AXIS, accel_x);
    float accel_y_mm = rev_to_mm(AxisEnum::Y_AXIS, accel_y);
    float block_accel = sqrt(accel_x_mm * accel_x_mm + accel_y_mm * accel_y_mm);

    auto target = current_position;
    target.x += rev_to_mm(AxisEnum::X_AXIS, revs_x);
    target.y += rev_to_mm(AxisEnum::Y_AXIS, revs_y);

    log_debug(PhaseStepping, "A: d(%f, %f), s(%f, %f, %f), e(%f, %f, %f), a(%f, %f, %f)",
        revs_x, revs_y, start_speed_x, start_speed_y, block_start_speed, end_speed_x, end_speed_y, block_end_speed, accel_x, accel_y, block_accel);

    Planner::buffer_raw_line(target, block_accel, block_nominal_speed, block_start_speed, block_end_speed, active_extruder);
    current_position = target;

    return duration;
}

static float plan_accel_over_dist_block(AxisEnum physical_axis, int direction,
    float start_speed, float end_speed, float revs) {
    assert(direction == 1 || direction == -1);
    assert(start_speed >= 0 && end_speed >= 0);
    assert(start_speed != end_speed);
    assert(revs > 0);

    // Plan a block with limited acceleration that moves from start speed to end
    // speed over revs revolutions. The assumption is that the printer already
    // accelerated to the start speed and there will be extra movement planned
    // after this block to deccelerate.

    float accel_rev = std::fabs(end_speed * end_speed - start_speed * start_speed) / (2 * revs);

    auto [revs_x, revs_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? direction * revs : 0,
        physical_axis == AxisEnum::Y_AXIS ? direction * revs : 0);
    auto [accel_x, accel_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? accel_rev : 0,
        physical_axis == AxisEnum::Y_AXIS ? accel_rev : 0);
    auto [start_speed_x, start_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? start_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? start_speed : 0);
    auto [end_speed_x, end_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? end_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? end_speed : 0);

    float start_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, start_speed_x);
    float start_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, start_speed_y);
    float block_start_speed = sqrt(start_speed_x_mm * start_speed_x_mm + start_speed_y_mm * start_speed_y_mm);

    float end_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, end_speed_x);
    float end_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, end_speed_y);
    float block_end_speed = sqrt(end_speed_x_mm * end_speed_x_mm + end_speed_y_mm * end_speed_y_mm);

    float block_nominal_speed = std::max(block_start_speed, block_end_speed);

    float accel_x_mm = rev_to_mm(AxisEnum::X_AXIS, accel_x);
    float accel_y_mm = rev_to_mm(AxisEnum::Y_AXIS, accel_y);
    float block_accel = sqrt(accel_x_mm * accel_x_mm + accel_y_mm * accel_y_mm);

    auto target = current_position;
    target.x += rev_to_mm(AxisEnum::X_AXIS, revs_x);
    target.y += rev_to_mm(AxisEnum::Y_AXIS, revs_y);

    log_debug(PhaseStepping, "Accel over dist block: dist (%f, %f), start (%f, %f, %f), end (%f, %f, %f), accel (%f, %f, %f)",
        revs_x, revs_y, start_speed_x, start_speed_y, block_start_speed, end_speed_x, end_speed_y, block_end_speed, accel_x, accel_y, block_accel);

    Planner::buffer_raw_line(target, block_accel, block_nominal_speed, block_start_speed, block_end_speed, active_extruder);
    current_position = target;

    return 2 * revs / (start_speed + end_speed);
}

static float plan_marker_move(AxisEnum physical_axis, int direction) {
    static const float MARKER_SPEED = 2; // rev/s
    static const float MAKER_ACCEL = 1000; // rev/s^2

    float duration = 0;

    duration += plan_accel_block(physical_axis, direction, 0, MARKER_SPEED, MAKER_ACCEL);
    duration += plan_accel_block(physical_axis, direction, MARKER_SPEED, 0, MAKER_ACCEL);
    duration += plan_accel_block(physical_axis, -direction, 0, MARKER_SPEED, MAKER_ACCEL);
    duration += plan_accel_block(physical_axis, -direction, MARKER_SPEED, 0, MAKER_ACCEL);

    return duration;
}

// Captures samples of movement of a given axis. The movement should be already
// planned and executed. The function yields samples of acceleration in the
// direction of the axis. The function returns a tuple of:
// - sampling frequency,
// - movement success flag,
// - accelerometer error.
static std::tuple<float, bool, PrusaAccelerometer::Error> capture_movement_samples(AxisEnum axis, int32_t timeout_ms,
    const std::function<void(float)> &yield_sample) {

    if (!wait_for_movement_state(phase_stepping::axis_states[axis], 300, [](phase_stepping::AxisState &s) {
            return phase_stepping::processing();
        })) {
        log_error(PhaseStepping, "Movement didn't start within timeout");
        return { 0, false, PrusaAccelerometer::Error::none };
    }

    auto start_ts = ticks_ms();

    PrusaAccelerometer accelerometer;
    if (PrusaAccelerometer::Error error = accelerometer.get_error(); error != PrusaAccelerometer::Error::none) {
        log_error(PhaseStepping, "Cannot initialize accelerometer %u", static_cast<unsigned>(error));
        return { 0, true, error };
    }
    accelerometer.clear();

    while (Planner::busy() && ticks_diff(ticks_ms(), start_ts) < timeout_ms) {
        PrusaAccelerometer::Acceleration sample;
        using GetSampleResult = PrusaAccelerometer::GetSampleResult;

        switch (accelerometer.get_sample(sample)) {
        case GetSampleResult::ok:
            yield_sample(project_to_axis(axis, sample));
            break;

        case GetSampleResult::buffer_empty:
            idle(true, true);
            break;

        case GetSampleResult::error: {
            const PrusaAccelerometer::Error error = accelerometer.get_error();
            log_error(PhaseStepping, "Accelerometer reading failed %u", static_cast<unsigned>(error));
            return { 0, true, error };
        }
        }
    }

    const PrusaAccelerometer::Error error = accelerometer.get_error();
    float sampling_freq = accelerometer.get_sampling_rate();

    if (ticks_diff(ticks_ms(), start_ts) >= timeout_ms) {
        log_error(PhaseStepping, "Timeout while capturing samples");
        return { sampling_freq, false, error };
    }

    return { sampling_freq, true, error };
}

void phase_stepping::reset_compensation(AxisEnum axis) {
    phase_stepping::axis_states[axis].forward_current.clear();
    phase_stepping::axis_states[axis].backward_current.clear();
}

// Return a tuple <motor_period_count, relevant_samples_count> that gives a
// number of full motor periods in the signal and corresponding signal count
std::optional<std::tuple<int, int>> matching_samples_count(int sample_count, float sampling_freq,
    AxisEnum axis, float speed) {
    float motor_period_duration = 1 / (speed * get_motor_steps(axis) / 4);
    float sampling_duration = sample_count / sampling_freq;
    int motor_period_count = int(sampling_duration / motor_period_duration);
    int relevant_samples_count = int(motor_period_count * motor_period_duration / sampling_duration * sample_count);

    if (relevant_samples_count >= sample_count) {
        log_error(PhaseStepping, "Not enough samples for calibration: %d/%d",
            relevant_samples_count, sample_count);
        return std::nullopt;
    }

    return std::tuple { motor_period_count, relevant_samples_count };
}

float phase_stepping::capture_samples(AxisEnum axis, float speed, float revs,
    const std::function<void(const PrusaAccelerometer::Acceleration &)> &yield_sample) {
    assert(speed > 0);

    Planner::synchronize();

    phase_stepping::AxisState &axis_state = phase_stepping::axis_states[axis];

    // Find move target that corresponds to given number of revs
    auto [measurement_revs, vibration_delay] = sample_capture_revs(revs, speed);
    int direction = revs > 0 ? 1 : -1;
    if (axis_state.inverted) {
        direction = -direction;
    }

    float axis_revs = direction * measurement_revs;
    float a_revs = axis == AxisEnum::X_AXIS ? axis_revs : 0;
    float b_revs = axis == AxisEnum::Y_AXIS ? axis_revs : 0;

    auto [x_revs, y_revs] = physical_to_logical(a_revs, b_revs);
    auto [x_speed, y_speed] = physical_to_logical(
        axis == AxisEnum::X_AXIS ? rev_to_mm(AxisEnum::X_AXIS, speed) : 0,
        axis == AxisEnum::Y_AXIS ? rev_to_mm(AxisEnum::Y_AXIS, speed) : 0);
    float d_x = rev_to_mm(AxisEnum::X_AXIS, x_revs);
    float d_y = rev_to_mm(AxisEnum::Y_AXIS, y_revs);

    float feedrate_mms = sqrt(x_speed * x_speed + y_speed * y_speed);

    plan_move_by(feedrate_mms, d_x, d_y);

    if (!wait_for_accel_end(axis_state)) {
        log_error(PhaseStepping, "Acceleration phase didn't ended within timeout");
        return 0;
    }

    gcode.dwell(vibration_delay * 1000, true);

    int counter = 0;
    PrusaAccelerometer accelerometer;
    if (PrusaAccelerometer::Error error = accelerometer.get_error(); error != PrusaAccelerometer::Error::none) {
        log_error(PhaseStepping, "Cannot initialize accelerometer %u", static_cast<unsigned>(error));
        return 0;
    }
    accelerometer.clear();
    while (axis_state.is_cruising.load()) {
        counter++;
        PrusaAccelerometer::Acceleration sample;
        using GetSampleResult = PrusaAccelerometer::GetSampleResult;
        switch (accelerometer.get_sample(sample)) {

        case GetSampleResult::ok:
            if (counter > 20) { // Be pessimistic about old samples in FIFOs
                yield_sample(sample);
            }
            break;

        case GetSampleResult::buffer_empty:
            idle(true, true);
            break;

        case GetSampleResult::error: {
            const PrusaAccelerometer::Error error = accelerometer.get_error();
            log_error(PhaseStepping, "Accelerometer reading failed %u", static_cast<unsigned>(error));
            return 0;
        }
        }
    }
    const PrusaAccelerometer::Error error = accelerometer.get_error();

    if (error != PrusaAccelerometer::Error::none) {
        log_error(PhaseStepping, "Accelerometer reading failed %u", static_cast<unsigned>(error));
        return 0;
    }

    return accelerometer.get_sampling_rate();
}

SamplesAnnotation phase_stepping::capture_param_sweep_samples(AxisEnum axis, float speed, float revs, int harmonic,
    float start_pha, float end_pha, float start_mag, float end_mag,
    const std::function<void(float)> &yield_sample) {
    assert(speed > 0);

    static const float TIME_MARGIN = 0.3; // Time to settle vibrations from acceleration

    Planner::synchronize();

    phase_stepping::AxisState &axis_state = phase_stepping::axis_states[axis];

    // Find move target that corresponds to given number of revs
    int direction = revs > 0 ? 1 : -1;
    if (axis_state.inverted) {
        direction = -direction;
    }
    revs = std::fabs(revs);

    axis_state.calibration_sweep = CalibrationSweep::build_for_motor(
        axis, harmonic, TIME_MARGIN * speed, revs,
        start_pha, end_pha, start_mag, end_mag);

    float start_marker = 0;
    start_marker = plan_no_movement_block(axis, direction, 0.1f);

    float signal_start = start_marker;
    signal_start += plan_marker_move(axis, direction);

    signal_start += plan_accel_block(axis, direction, 0, speed);

    float signal_end = signal_start;
    signal_end += plan_constant_movement_block(axis, direction, speed, revs + 2 * TIME_MARGIN * speed);

    float end_marker = signal_end;
    end_marker += plan_accel_block(axis, direction, speed, 0);
    end_marker += plan_no_movement_block(axis, direction, 0.1f);

    plan_marker_move(axis, direction);
    plan_no_movement_block(axis, direction, 0.1f);

    // Adjust the signal start and end to account for extra time to settle
    // vibrations
    signal_start += TIME_MARGIN;
    signal_end -= TIME_MARGIN;

    int timeout_ms = 1.5 * 1000 * end_marker;
    auto [sampling_freq, movement_ok, acc_error] = capture_movement_samples(axis, timeout_ms, yield_sample);

    return {
        .sampling_freq = sampling_freq,
        .movement_ok = movement_ok,
        .accel_error = acc_error,
        .start_marker = start_marker,
        .end_marker = end_marker,
        .signal_start = signal_start,
        .signal_end = signal_end
    };
}

SamplesAnnotation phase_stepping::capture_speed_sweep_samples(AxisEnum axis,
    float start_speed, float end_speed, float revs,
    const std::function<void(float)> &yield_sample) {

    assert(start_speed >= 0 && end_speed >= 0);
    assert(start_speed != 0 || start_speed != end_speed);

    Planner::synchronize();

    int direction = revs > 0 ? 1 : -1;
    if (axis_states[axis].inverted) {
        direction = -direction;
    }
    revs = std::fabs(revs);

    float start_marker = 0;
    start_marker += plan_no_movement_block(axis, direction, 0.1f);

    float signal_start = start_marker;

    signal_start += plan_marker_move(axis, direction);
    signal_start += plan_no_movement_block(axis, direction, 0.3f);
    signal_start += plan_accel_block(axis, direction, 0, start_speed);

    float signal_end = signal_start;
    signal_end += plan_accel_over_dist_block(axis, direction, start_speed, end_speed, std::fabs(revs));

    float end_marker = signal_end;

    end_marker += plan_accel_block(axis, direction, end_speed, 0);
    end_marker += plan_no_movement_block(axis, direction, 0.1f);
    plan_marker_move(axis, direction);
    plan_no_movement_block(axis, direction, 0.3f);

    int timeout_ms = 1.5 * 1000 * end_marker;
    auto [sampling_freq, movement_ok, acc_error] = capture_movement_samples(axis, timeout_ms, yield_sample);

    return {
        .sampling_freq = sampling_freq,
        .movement_ok = movement_ok,
        .accel_error = acc_error,
        .start_marker = start_marker,
        .end_marker = end_marker,
        .signal_start = signal_start,
        .signal_end = signal_end
    };
}

std::vector<float> phase_stepping::analyze_resonance(AxisEnum axis,
    float speed, float revs, const std::vector<int> &requested_harmonics) {
    const int expected_max_sample_count = 1500 * speed * std::abs(revs);

    std::vector<float> signal;
    signal.reserve(expected_max_sample_count);
    float sampling_freq = capture_samples(axis, speed, revs, [&](const auto &sample) {
        if (signal.size() < signal.capacity()) {
            signal.push_back(project_to_axis(axis, sample));
        }
    });

    if (sampling_freq < 1100 || sampling_freq > 1500) {
        log_error(PhaseStepping, "Sampling freq out of range: %f", sampling_freq);
        return {};
    }

    auto msc_r = matching_samples_count(
        signal.size(), sampling_freq, axis, speed);

    if (!msc_r.has_value()) {
        log_error(PhaseStepping, "Cannot analyze resonance, no matching sample count");
        return {};
    }
    auto [motor_period_count, samples_count] = *msc_r;

    if (samples_count < 3) {
        log_error(PhaseStepping, "Cannot analyze resonance, too short signal: %d", samples_count);
        return {};
    }

    signal.resize(samples_count);

    std::vector<float> res;
    res.reserve(requested_harmonics.size());

    for (auto n : requested_harmonics) {
        std::size_t harm = n * motor_period_count;
        assert(harm < signal.size());
        res.push_back(dft_n_mag(signal, harm) / motor_period_count);

        idle(true, true);
    }

    return { res };
}

// Execution of calibration phase can be nicely divided into separate functions,
// however, these functions share a common context. This class provides the
// context for the (otherwise free-standing) functions.
class CalibrationPhaseExecutor {
    static constexpr int RETRY_COUNT = 4;

    AxisEnum _axis;
    const CalibrationPhase &_phase_config;
    CalibrateAxisHooks &_hooks;
    int _progress = 0;

    int _progress_tick_count() const {
        return 2 * _phase_config.iteration_count;
    }

    // Iterate search for forward and backward dimension; leaves the optimal
    // result in the correction table. Returns forward and backward score
    template <typename F>
        requires requires(F f, MotorPhaseCorrection &mpc, int i, float x) { f(mpc, i, x); }
    std::optional<std::tuple<float, float>> _run_simultaneous_search(
#define IDLE()                                                                                \
    do {                                                                                      \
        idle(true, true);                                                                     \
        if (_hooks.on_idle() == phase_stepping::CalibrateAxisHooks::ContinueOrAbort::Abort) { \
            return {};                                                                        \
        }                                                                                     \
    } while (0)

        InterruptableGoldenSearch forward_search,
        InterruptableGoldenSearch backward_search,
        int iterations,
        F apply_x) {
        auto &axis_state = phase_stepping::axis_states[_axis];

        // Calibration is quite CPU-heavy, we gotta release the processor to do other stuff from time to time
        // Also GUI thread is waiting for marlin server acks, so we gotta do marlin idling
        // That's why the idles are here

        for (int i = 0; i != iterations; i++) {
            forward_search.run();
            backward_search.run();

            axis_state.forward_current.modify_correction([&](auto &table) {
                apply_x(table, 0, forward_search.x());
            });

            IDLE();

            axis_state.backward_current.modify_correction([&](auto &table) {
                apply_x(table, 1, backward_search.x());
            });

            IDLE();

            for (int retries = 0; retries <= RETRY_COUNT; retries++) {
                if (retries == RETRY_COUNT) {
                    log_error(PhaseStepping, "Out of retries in argument search");
                    return std::nullopt;
                }
                if (retries != 0) {
                    move_to_calibration_start(_axis, _phase_config);
                }

                auto b_res = phase_stepping::analyze_resonance(_axis,
                    _phase_config.speed, _phase_config.revs, { _phase_config.harmonic });

                IDLE();

                auto f_res = phase_stepping::analyze_resonance(_axis,
                    _phase_config.speed, -_phase_config.revs, { _phase_config.harmonic });

                IDLE();

                if (!f_res.empty() && !b_res.empty()) {
                    forward_search.submit(f_res[0]);
                    backward_search.submit(b_res[0]);
                    break;
                }
                log_error(PhaseStepping, "Resonance analysis failed in argument search");
            }
            _hooks.on_calibration_phase_progress(100 * (_progress++) / _progress_tick_count());

            IDLE();
        }

        // Apply the result
        axis_state.forward_current.modify_correction([&](auto &table) {
            apply_x(table, 0, forward_search.arg_min());
        });
        IDLE();

        axis_state.backward_current.modify_correction([&](auto &table) {
            apply_x(table, 1, backward_search.arg_min());
        });
        IDLE();

#ifdef SERIAL_DEBUG
        SERIAL_ECHO("Forward value: ");
        SERIAL_ECHO(forward_search.min());
        SERIAL_ECHO(", backward value: ");
        SERIAL_ECHOLN(backward_search.min());
#endif
        return { { forward_search.min(), backward_search.min() } };
#undef IDLE
    }

    const SpectralItem &_get_fwd_item() const {
        return axis_states[_axis].forward_current.get_correction()[_phase_config.harmonic];
    }

    const SpectralItem &_get_bwd_item() const {
        return axis_states[_axis].backward_current.get_correction()[_phase_config.harmonic];
    }

public:
    CalibrationPhaseExecutor(
        AxisEnum axis,
        const CalibrationPhase &phase_config, CalibrateAxisHooks &hooks)
        : _axis(axis)
        , _phase_config(phase_config)
        , _hooks(hooks) {}

    std::optional<std::tuple<float, float>> baseline() {
        for (int retries = 0; retries != RETRY_COUNT; retries++) {
            auto b_res = phase_stepping::analyze_resonance(_axis,
                _phase_config.speed, _phase_config.revs, { { _phase_config.harmonic } });
            auto f_res = phase_stepping::analyze_resonance(_axis,
                _phase_config.speed, -_phase_config.revs, { { _phase_config.harmonic } });

            if (!f_res.empty() && !b_res.empty()) {
#ifdef SERIAL_DEBUG
                SERIAL_ECHO("Baseline: ");
                SERIAL_ECHO(f_res[0]);
                SERIAL_ECHO(", ");
                SERIAL_ECHOLN(b_res[0]);
#endif
                return { { f_res[0], b_res[0] } };
            }
        }

        log_error(PhaseStepping, "Cannot obtain baseline measurement");
        return std::nullopt;
    }

    // Return success flags, leave axis corrections tables in the final
    // destination
    std::optional<std::tuple<float, float>> run() {
        // Find phase
        const float initial_f_pha = _phase_config.pha.value_or(_get_fwd_item().pha);
        const float initial_f_mag = _phase_config.mag.value_or(_get_fwd_item().mag);
        const float initial_b_pha = _phase_config.pha.value_or(_get_bwd_item().pha);
        const float initial_b_mag = _phase_config.mag.value_or(_get_bwd_item().mag);
        const float pha_range = _phase_config.pha_window / 2.f;
        auto phase_res = _run_simultaneous_search(
            { initial_f_pha - pha_range, initial_f_pha + pha_range, "forward-pha" },
            { initial_b_pha - pha_range, initial_b_pha + pha_range, "backward-pha" },
            _phase_config.iteration_count,
            [&](MotorPhaseCorrection &mph, int i, float pha) {
                mph[_phase_config.harmonic] = {
                    .mag = (i == 0) ? initial_f_mag : initial_b_mag,
                    .pha = pha
                };
            });
        if (!phase_res.has_value()) {
            return std::nullopt;
        }

        // Find magnitude
        const float mag_range = _phase_config.mag_window / 2.f;
        auto mag_res = _run_simultaneous_search(
            { initial_f_mag - mag_range, initial_f_mag + mag_range, "forward-mag" },
            { initial_b_mag - mag_range, initial_b_mag + mag_range, "backward-mag" },
            _phase_config.iteration_count,
            [&](MotorPhaseCorrection &mph, int i, float mag) {
                mph[_phase_config.harmonic].mag = mag;
            });
        if (!mag_res.has_value()) {
            return std::nullopt;
        }

        return { mag_res };
    }
};

std::optional<std::tuple<MotorPhaseCorrection, MotorPhaseCorrection>>
phase_stepping::calibrate_axis(AxisEnum axis, CalibrateAxisHooks &hooks) {
    std::optional<std::tuple<MotorPhaseCorrection, MotorPhaseCorrection>> r;

    reset_compensation(axis);

    phase_stepping::EnsureEnabled _;

    hooks.on_initial_movement();
    move_to_calibration_start(axis, printer_calibration_config.phases[0]);
    Planner::synchronize();

    hooks.set_calibration_phases_count(printer_calibration_config.phases.size());
    for (std::size_t phase_i = 0; phase_i != printer_calibration_config.phases.size(); phase_i++) {
        const CalibrationPhase &calib_phase = printer_calibration_config.phases[phase_i];

        hooks.on_enter_calibration_phase(phase_i);

        auto executor = CalibrationPhaseExecutor(axis, calib_phase, hooks);
        auto baseline_res = executor.baseline();
        if (!baseline_res.has_value()) {
            return r;
        }
        auto [baseline_f, baseline_b] = *baseline_res;

        auto calib_res = executor.run();
        if (!calib_res.has_value()) {
            return r;
        }
        auto [min_f, min_b] = *calib_res;

        hooks.on_calibration_phase_result(min_f / baseline_f, min_b / baseline_b);
    }
    hooks.on_termination();

    r.emplace();
    std::get<0>(*r) = phase_stepping::axis_states[axis].forward_current.get_correction();
    std::get<1>(*r) = phase_stepping::axis_states[axis].backward_current.get_correction();
    return r;
}
