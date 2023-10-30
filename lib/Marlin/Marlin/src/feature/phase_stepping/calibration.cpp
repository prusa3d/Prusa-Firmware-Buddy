#include "calibration.hpp"
#include "phase_stepping.hpp"
#include "calibration_config.hpp"
#include "lut.hpp"

#include <log.h>
#include <module/planner.h>
#include <module/motion.h>

#include <vector>
#include <cmath>
#include <numbers>

#ifdef PHASE_STEPPING

using namespace phase_stepping;

LOG_COMPONENT_REF(PhaseStepping);

// Temporary debugging to Marlin serial for convenience
#define SERIAL_DEBUG

static const char* fixed_repr(float number, int n = 5) {
    static char buffer[32];
    snprintf(buffer, 32, "%.*f", n, number);
    return buffer;
}


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
        log_debug(PhaseStepping, "%s: it %d, bracket (%f, %f) = (%f, %f)",
            _debug_name, _iterations, _c, _d, _fc, _fd);

        #ifdef SERIAL_DEBUG
            SERIAL_ECHO(_debug_name);
            SERIAL_ECHO(": it ");
            SERIAL_ECHO(_iterations);
            SERIAL_ECHO(", bracket (");
            SERIAL_ECHO(fixed_repr(_c));
            SERIAL_ECHO(", ");
            SERIAL_ECHO(fixed_repr(_d));
            SERIAL_ECHO(") = (");
            SERIAL_ECHO(fixed_repr(_fc));
            SERIAL_ECHO(", ");
            SERIAL_ECHO(fixed_repr(_fd));
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
        : _a(a), _b(b), _debug_name(name)
    {}

    float x() const {
        return _requested_x;
    }

    void submit(float fx) {
        _result = fx;
    }

    void run() {
        switch(_phase) {
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
                    SERIAL_ECHO(fixed_repr(_c));
                    SERIAL_ECHO(", ");
                    SERIAL_ECHO(fixed_repr(_d));
                    SERIAL_ECHO(") = (");
                    SERIAL_ECHO(fixed_repr(_fc));
                    SERIAL_ECHO(", ");
                    SERIAL_ECHO(fixed_repr(_fd));
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
        return {x, y};
    #endif
}

static float rev_to_mm(AxisEnum axis, float revs) {
    static constinit std::array< float, SUPPORTED_AXIS_COUNT > FACTORS = []() consteval {
        static_assert(SUPPORTED_AXIS_COUNT <= 3);

        int STEPS_PER_UNIT[] = DEFAULT_AXIS_STEPS_PER_UNIT;
        int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };

        std::array< float, SUPPORTED_AXIS_COUNT > ret;
        for (int i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
            ret[i] = float(MICROSTEPS[i]) / float(STEPS_PER_UNIT[i]);
        }
        return ret;
    }();
    return revs * get_motor_steps(axis) * FACTORS[axis];
}

// Wait for a state of a given axis specified by a predicate. Returns true on
// success, false on timeout
template <typename Pred>
static bool wait_for_movement_state(phase_stepping::AxisState& axis_state,
    int timeout_ms, Pred pred)
{
    auto start_time = ticks_ms();
    while (!(pred(axis_state))) {
        if (ticks_diff(ticks_ms(), start_time) > timeout_ms)
            return false;
        osDelay(1);
    }
    return true;
}

// Wait for movement start of a given axis. Returns true on success, false on
// timeout
static bool wait_for_movement_start(phase_stepping::AxisState& axis_state,
    int timeout_ms = 1000)
{
    return wait_for_movement_state(axis_state, timeout_ms, [](phase_stepping::AxisState& s){
        return s.is_moving.load();
    });
}

// Wait for end of acceleration phase of a given axis. Returns true on success,
// false on timeout
static bool wait_for_accel_end(phase_stepping::AxisState& axis_state,
    int timeout_ms = 1000)
{
    return wait_for_movement_state(axis_state, timeout_ms, [](phase_stepping::AxisState& s){
        return s.is_cruising.load();
    });
}

// Computes a pseudo-projection of one vector to another. The length of
// direction vector is not normalized.
static float pseudo_project(std::tuple<float, float> what, std::tuple<float, float> dir) {
    return std::get<0>(what) * std::get<0>(dir) + std::get<1>(what) * std::get<1>(dir);
}

static float project_to_axis(AxisEnum axis, const PrusaAccelerometer::Acceleration& sample) {
    #ifdef COREXY
        std::pair<float, float> proj_dir;
        if (axis == AxisEnum::X_AXIS)
            proj_dir = {M_SQRT1_2, M_SQRT1_2};
        else if (axis == AxisEnum::Y_AXIS)
            proj_dir = {-M_SQRT1_2, M_SQRT1_2};
        else
            bsod("Unsupported axis");

        return pseudo_project({sample.val[0], sample.val[1]}, proj_dir);
    #else
        if (axis == AxisEnum::X_AXIS)
            return sample.val[0];
        else if (axis == AxisEnum::Y_AXIS)
            return sample.val[1];
        else
            bsod("Unsupported axis");
    #endif
}

// Naively compute n-th bin of DFT and return its amplitude
static float dft_n_mag(const std::vector<float> signal, int n) {
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

static void move_to_calibration_start(AxisEnum axis, const PrinterCalibrationConfig& config) {
    auto [measurement_revs, _] = sample_capture_revs(config.calib_revs, config.phases[0].speed);

    float a_revs = axis == AxisEnum::A_AXIS ? measurement_revs : 0;
    float b_revs = axis == AxisEnum::B_AXIS ? measurement_revs : 0;
    const auto [d_rot_x, d_rot_y] = physical_to_logical(a_revs, b_revs);
    const float dx = rev_to_mm(AxisEnum::X_AXIS, d_rot_x);
    const float dy = rev_to_mm(AxisEnum::Y_AXIS, d_rot_y);

    const float target_x = X_BED_SIZE / 2.f - dx / 2.f;
    const float target_y = Y_BED_SIZE / 2.f - dy / 2.f;

    do_blocking_move_to_xy(target_x, target_y);
}

static void reset_compensation(AxisEnum axis) {
    phase_stepping::axis_states[axis]->forward_current = {};
    phase_stepping::axis_states[axis]->backward_current = {};
}

// Return a tuple <motor_period_count, relevant_samples_count> that gives a
// number of full motor periods in the signal and corresponding signal count
std::tuple<int, int> matching_samples_count(int sample_count, float sampling_freq,
    AxisEnum axis, float speed)
{
    float motor_period_duration = 1 / (speed * get_motor_steps(axis) / 4);
    float sampling_duration = sample_count / sampling_freq;
    int motor_period_count = int(sampling_duration / motor_period_duration);
    int relevant_samples_count = int(motor_period_count * motor_period_duration / sampling_duration * sample_count);

    assert(relevant_samples_count < sample_count);
    return {motor_period_count, relevant_samples_count};
}

float phase_stepping::capture_samples(AxisEnum axis, float speed, float revs,
    std::function<void(const PrusaAccelerometer::Acceleration&)> yield_sample)
{
    assert(speed > 0);

    Planner::synchronize();

    phase_stepping::AxisState &axis_state = *phase_stepping::axis_states[axis];

    // Find move target that corresponds to given number of revs
    auto [measurement_revs, vibration_delay] = sample_capture_revs(revs, speed);
    int direction = revs > 0 ? 1 : -1;
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

    bool success;
    success = wait_for_movement_start(axis_state);
    if (!success)
        return 0;
    success = wait_for_accel_end(axis_state);
    if (!success)
        return 0;

    delay(vibration_delay * 1000);

    int counter = 0;
    PrusaAccelerometer accelerometer;
    if (accelerometer.get_error() != PrusaAccelerometer::Error::none) {
        log_debug(PhaseStepping, "Cannot initialize accelerometer %d", accelerometer.get_error());
        return 0;
    }
    accelerometer.clear();
    while (axis_state.is_cruising.load()) {
        counter++;
        PrusaAccelerometer::Acceleration sample;
        int has_new_sample = accelerometer.get_sample(sample);
        if (has_new_sample && counter > 20) { // Be pessimistic about old samples in FIFOs
            yield_sample(sample);
        }
    }
    if (accelerometer.get_error() != PrusaAccelerometer::Error::none) {
        log_debug(PhaseStepping, "Accelerometer reading failed %d", accelerometer.get_error());
        return 0;
    }
    return accelerometer.get_sampling_rate();
}

std::optional<std::vector<float>> phase_stepping::analyze_resonance(AxisEnum axis,
    float speed, float revs, std::vector<int> requested_harmonics)
{
    const int expected_max_sample_count = 1500 * speed * std::abs(revs);

    std::vector<float> signal;
    signal.reserve(expected_max_sample_count);
    float sampling_freq = capture_samples(axis, speed, revs, [&](const auto& sample) {
        if (signal.size() < signal.capacity())
            signal.push_back(project_to_axis(axis, sample));
    });

    log_debug(PhaseStepping, "Sampling freq: %f", sampling_freq);
    if (sampling_freq < 1100 || sampling_freq > 1500)
        return std::nullopt;

    auto [motor_period_count, samples_count] = matching_samples_count(
        signal.size(), sampling_freq, axis, speed);

    if (samples_count < 3) {
        log_debug(PhaseStepping, "Too short signal: %d", samples_count);
        return std::nullopt;
    }

    signal.resize(samples_count);

    std::vector<float> res;
    for (auto n : requested_harmonics) {
        std::size_t harm = n * motor_period_count;
        assert(harm < signal.size());
        if (harm <= 0 || harm >= signal.size() - 1)
            res.push_back(dft_n_mag(signal, n * motor_period_count));
        else {
            float a1 = dft_n_mag(signal, n * motor_period_count - 1);
            float a2 = dft_n_mag(signal, n * motor_period_count);
            float a3 = dft_n_mag(signal, n * motor_period_count + 1);
            res.push_back((a1 + a2 + a3) / 3.f);
        }
    }

    return {res};
}

// Execution of calibration phase can be nicely divided into separate functions,
// however, these functions share a common context. This class provides the
// context for the (otherwise free-standing) functions.
class CalibrationPhaseExecutor {
    static constexpr int RETRY_COUNT = 4;

    AxisEnum _axis;
    const PrinterCalibrationConfig& _printer_config;
    const CalibrationPhase& _phase_config;
    CalibrationReporterBase& _reporter;
    int _progress = 0;

    int _progress_tick_count() const {
        return 2 * _phase_config.iteration_count;
    }

    // Iterate search for forward and backward dimension; leaves the optimal
    // result in the correction table. Returns forward and backward score
    template <typename F>
        requires requires(F f, MotorPhaseCorrection& mpc, int i, float x) { f(mpc, i, x); }
    std::optional<std::tuple<float, float>> _run_simultaneous_search(
        InterruptableGoldenSearch forward_search,
        InterruptableGoldenSearch backward_search,
        int iterations,
        F apply_x)
    {
        auto &axis_state = *phase_stepping::axis_states[_axis];

        for (int i = 0; i != iterations; i++) {
            forward_search.run();
            backward_search.run();

            axis_state.forward_current.modify_correction([&](auto& table){
                apply_x(table, 0, forward_search.x());
            });
            axis_state.backward_current.modify_correction([&](auto& table){
                apply_x(table, 1, backward_search.x());
            });

            for (int retries = 0; retries <= RETRY_COUNT; retries++) {
                if (retries == RETRY_COUNT)
                    return std::nullopt;
                if (retries != 0)
                    move_to_calibration_start(_axis, _printer_config);

                auto b_res = phase_stepping::analyze_resonance(_axis,
                    _phase_config.speed, _printer_config.calib_revs, {{_phase_config.harmonic}});


                auto f_res = phase_stepping::analyze_resonance(_axis,
                    _phase_config.speed, -_printer_config.calib_revs, {{_phase_config.harmonic}});


                if (f_res.has_value() && b_res.has_value()) {
                    forward_search.submit((*f_res)[0]);
                    backward_search.submit((*b_res)[0]);
                    break;
                }
            }
            _reporter.on_calibration_phase_progress(100 * (_progress++) / _progress_tick_count());
        }

        // Apply the result
        axis_state.forward_current.modify_correction([&](auto& table){
            apply_x(table, 0, forward_search.arg_min());
        });
        axis_state.backward_current.modify_correction([&](auto& table){
            apply_x(table, 1, backward_search.arg_min());
        });
        return {{forward_search.min(), backward_search.min()}};
    }

    const SpectralItem& _get_fwd_item() const {
        return axis_states[_axis]->forward_current.get_correction()[_phase_config.harmonic];
    }

    const SpectralItem& _get_bwd_item() const {
        return axis_states[_axis]->backward_current.get_correction()[_phase_config.harmonic];
    }
public:
    CalibrationPhaseExecutor(
        AxisEnum axis, const PrinterCalibrationConfig& printer_config,
        const CalibrationPhase& phase_config, CalibrationReporterBase& reporter)
        : _axis(axis),
          _printer_config(printer_config),
          _phase_config(phase_config),
          _reporter(reporter)
    {}

    std::optional<std::tuple<float, float>> baseline() {
        for (int retries = 0; retries != RETRY_COUNT; retries++) {
            auto b_res = phase_stepping::analyze_resonance(_axis,
                _phase_config.speed, _printer_config.calib_revs, {{_phase_config.harmonic}});
            auto f_res = phase_stepping::analyze_resonance(_axis,
                _phase_config.speed, -_printer_config.calib_revs, {{_phase_config.harmonic}});

            if (f_res.has_value() && b_res.has_value()) {
                return {{(*f_res)[0], (*b_res)[0]}};
            }
        }

        return std::nullopt;
    }

    // Return success flags, leave axis corrections tables in the final
    // destination
    std::optional<std::tuple<float, float>> run() {
        // Find phase
        float initial_f_pha = _phase_config.pha.value_or(_get_fwd_item().pha);
        float initial_f_mag = _phase_config.mag.value_or(_get_fwd_item().mag);
        float initial_b_pha = _phase_config.pha.value_or(_get_bwd_item().pha);
        float initial_b_mag = _phase_config.mag.value_or(_get_bwd_item().mag);
        float pha_range = _phase_config.pha_window / 2.f;
        auto phase_res = _run_simultaneous_search(
            { initial_f_pha - pha_range, initial_f_pha + pha_range, "forward-pha"},
            { initial_b_pha - pha_range, initial_b_pha + pha_range, "backward-pha"},
            _phase_config.iteration_count,
            [&](MotorPhaseCorrection& mph, int i, float pha) {
                mph[_phase_config.harmonic] = {
                    .mag = (i == 0) ? initial_f_mag : initial_b_mag,
                    .pha = pha
                };
            });
        if (!phase_res.has_value())
            return std::nullopt;

        // Find magnitude
        float mag_range = _phase_config.mag_window / 2.f;
        auto mag_res = _run_simultaneous_search(
            { initial_f_mag - mag_range, initial_f_mag + mag_range, "forward-mag"},
            { initial_b_mag - mag_range, initial_b_mag + mag_range, "backward-mag"},
            _phase_config.iteration_count,
            [&](MotorPhaseCorrection& mph, int i, float mag) {
                mph[_phase_config.harmonic].mag = mag;
            });
        if (!mag_res.has_value())
            return std::nullopt;

        return {mag_res};
    }
};


std::optional<std::tuple<MotorPhaseCorrection, MotorPhaseCorrection>>
    phase_stepping::calibrate_axis(AxisEnum axis, CalibrationReporterBase& reporter)
{
    const auto config = get_printer_calibration_config();

    reset_compensation(axis);

    phase_stepping::EnsureEnabled _;

    reporter.on_initial_movement();
    move_to_calibration_start(axis, config);
    Planner::synchronize();

    reporter.set_calibration_phases_count(config.phases.size());
    for (std::size_t phase_i = 0; phase_i != config.phases.size(); phase_i++) {
        const CalibrationPhase& calib_phase = config.phases[phase_i];

        reporter.on_enter_calibration_phase(phase_i);

        auto executor = CalibrationPhaseExecutor(axis, config, calib_phase, reporter);
        auto baseline_res = executor.baseline();
        if (!baseline_res.has_value()) {
            return std::nullopt;
        }
        auto [baseline_f, baseline_b] = *baseline_res;

        auto calib_res = executor.run();
        if (!calib_res.has_value()) {
            return std::nullopt;
        }
        auto [min_f, min_b] = *calib_res;

        reporter.on_calibration_phase_result(min_f / baseline_f, min_b / baseline_b);
    }

    auto forward_correction = phase_stepping::axis_states[axis]->forward_current.get_correction();
    auto backward_correction = phase_stepping::axis_states[axis]->backward_current.get_correction();

    return {{forward_correction, backward_correction}};
}

#endif // ifdef PHASE_STEPPING
