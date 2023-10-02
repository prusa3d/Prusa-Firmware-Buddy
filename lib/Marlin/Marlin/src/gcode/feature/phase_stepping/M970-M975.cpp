#include "log.h"
#include "module/prusa/accelerometer.h"
#include <inc/MarlinConfig.h>
#include <gcode/gcode.h>
#include <module/planner.h>
#include <module/motion.h>
#include <feature/precise_stepping/precise_stepping.hpp>
#include <feature/phase_stepping/phase_stepping.hpp>
#include <string_view>
#include <charconv>

LOG_COMPONENT_REF(PhaseStepping);

using namespace std::literals;

static constexpr std::array< std::pair< AxisEnum, char>, 3 > SUPPORTED_AXES = {
    {
        {X_AXIS, 'X'},
        {Y_AXIS, 'Y'}
    }
};

// This is lambda on purpose so it can be passed around
static auto print_error = [](auto... args) {
    serial_error_start();
    (SERIAL_ECHO(args), ...);
    SERIAL_CHAR('\n');
};

bool is_one_of(char c, std::string_view sv) {
    for (char x : sv)
        if (c == x)
            return true;
    return false;
}

class DisableBusyReport {
    bool original_state = false;
public:
    DisableBusyReport() {
        original_state = suspend_auto_report;
        suspend_auto_report = true;
    }

    ~DisableBusyReport() {
        suspend_auto_report = original_state;
    }
};


/**
 * @brief Enable phase stepping for axis
 *
 * - valid axes X, Y
 */
void GcodeSuite::M970() {
    planner.synchronize();
    for ( auto [axis, letter] : SUPPORTED_AXES) {
        if (!parser.seen(letter))
            continue;

        auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis;
        if (PreciseStepping::physical_axis_step_generator_types & enable_mask)
            continue;

        phase_stepping::enable_phase_stepping(axis);
        PreciseStepping::physical_axis_step_generator_types |= enable_mask;
    }
}

/**
 * @brief Disable phase stepping for axis
 *
 * - valid axes X, Y
 */
void GcodeSuite::M971() {
    planner.synchronize();
    for ( auto [axis, letter] : SUPPORTED_AXES) {
        if (!parser.seen(letter))
            continue;

        auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis;
        if (!(PreciseStepping::physical_axis_step_generator_types & enable_mask))
            continue;

        phase_stepping::disable_phase_stepping(axis);
        PreciseStepping::physical_axis_step_generator_types &= ~enable_mask;
    }
}

/**
 * @brief Retrieve current correction
 *
 * Outputs the current correction table in format: `<axis>, <direction>,
 * <index>, <mag>, <pha>` per line. Multiple axes can be specified.
 **/
void GcodeSuite::M972() {
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (!parser.seen(letter))
            continue;
        const phase_stepping::AxisState& axis_state = phase_stepping::axis_states[axis];
        for (char dir : "FB"sv) {
            if (!parser.seen(letter))
                continue;

            const phase_stepping::CorrectedCurrentLut& lut = dir == 'F'
                ? axis_state.forward_current : axis_state.backward_current;

            const auto& table = lut.get_correction();
            for (size_t i = 0; i != table.size(); i++ ) {
                SERIAL_ECHO(letter);
                SERIAL_ECHO(", ");
                SERIAL_ECHO(dir);
                SERIAL_ECHO(", ");
                SERIAL_ECHO(i);
                SERIAL_ECHO(", ");
                SERIAL_ECHO(table[i].mag);
                SERIAL_ECHO(", ");
                SERIAL_ECHO(table[i].pha);
                SERIAL_ECHO('\n');
            }
        }
    }
}

static std::vector<std::pair<float, float>> parse_pairs(std::string_view str) {
    std::vector<std::pair<float, float>> pairs;

    while (!str.empty()) {
        size_t comma_pos = str.find(',');
        if (comma_pos == std::string_view::npos) {
            print_error("Malformed input: missing comma");
            return {}; // Return an empty vector on error
        }

        char* end_ptr;
        float first = std::strtof(str.data(), &end_ptr);
        if (end_ptr != str.data() + comma_pos) {
            print_error("Malformed input: unable to parse first value of a pair");
            return {}; // Return an empty vector on error
        }

        str.remove_prefix(comma_pos + 1);

        size_t space_pos = str.find(' ');

        float second = std::strtof(str.data(), &end_ptr);
        auto expected_end_ptr = space_pos != std::string_view::npos ? str.data() + space_pos : str.data() + str.size();
        if (end_ptr != expected_end_ptr) {
            print_error("Malformed input: unable to parse second value of a pair");
            return {}; // Return an empty vector on error
        }

        pairs.emplace_back(first, second);

        str.remove_prefix(space_pos != std::string_view::npos ? space_pos + 1 : str.size());
    }

    return pairs;
}

/**
 * @brief Set single entry for the current correction table.
 *
 * Parameters: String argument in format: <X/Y><F/B><list of mag,phase pairs separated by space>
 **/
void GcodeSuite::M973() {
    std::string_view str_arg {parser.string_arg};
    if (str_arg.size() < 3 || !is_one_of(str_arg[0], "XY") || !is_one_of(str_arg[1], "FB")) {
        print_error("Invalid format; should be <X/Y><F/B><list of mag,phase pairs separated by space");
    }

    AxisEnum axis = str_arg[0] == 'X' ? AxisEnum::X_AXIS : AxisEnum::Y_AXIS;
    phase_stepping::AxisState& axis_state = phase_stepping::axis_states[axis];
    phase_stepping::CorrectedCurrentLut& lut =
        str_arg[1] == 'F'
                ? axis_state.forward_current
                : axis_state.backward_current;

    str_arg.remove_prefix(2);
    auto data = parse_pairs(str_arg);

    if (data.empty())
        return;

    lut.modify_correction([&](auto& table) {
        for (size_t n = 0; n != table.size(); n++) {
            if (n < data.size()) {
                SERIAL_ECHO("Setting ");
                SERIAL_ECHO(n);
                SERIAL_ECHO(": ");
                SERIAL_ECHO(data[n].first);
                SERIAL_ECHO(", ");
                SERIAL_ECHOLN(data[n].second);
                table[n] = phase_stepping::SpectralItem{
                    .mag = data[n].first,
                    .pha = data[n].second
                };
            } else {
                table[n] = phase_stepping::SpectralItem{
                    .mag = 0,
                    .pha = 0
                };
            }
        }
    });
}

// Given change in physical space, return change in logical axis.
// - for cartesian this is identity,
// - for CORE XY it converts (A, B) to (X, Y)
static std::pair< double, double > physical_to_logical(double x, double y) {
    #ifdef COREXY
        return {
            (x + y) / 2,
            (x - y) / 2
        };
    #else
        return {x, y};
    #endif
}

static double rev_to_mm(int axis, double revs) {
    static constexpr int STEPS_PER_UNIT[] = DEFAULT_AXIS_STEPS_PER_UNIT;
    static constexpr int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };
    #ifdef HAS_LDO_400_STEP
        static constexpr double STEPS = 400.0;
    #else
        static constexpr double STEPS = 200.0;
    #endif
    return revs * STEPS * MICROSTEPS[axis] / STEPS_PER_UNIT[axis];
}

template < typename YieldError >
static bool accelerometer_ok(PrusaAccelerometer& acc, YieldError yield_error) {
    PrusaAccelerometer::Error error = acc.get_error();
    switch (error) {
        case PrusaAccelerometer::Error::none:
            return true;
        case PrusaAccelerometer::Error::communication:
            yield_error("accelerometer communication");
            return false;
        case PrusaAccelerometer::Error::no_active_tool:
            yield_error("no active tool");
            return false;
        case PrusaAccelerometer::Error::busy:
            yield_error("busy");
            return false;
        case PrusaAccelerometer::Error::corrupted_dwarf_overflow:
            yield_error("dwarf overflow");
            return false;
        case PrusaAccelerometer::Error::corrupted_buddy_overflow:
            yield_error("buddy overflow");
            return false;
        case PrusaAccelerometer::Error::corrupted_transmission_error:
            yield_error("dwarf transmission error");
            return false;
        case PrusaAccelerometer::Error::corrupted_sample_overrun:
            yield_error("overrun");
            return false;
        case PrusaAccelerometer::Error::corrupted_buddy_overflow:
            yield_error("corrupted_buddy_overflow");
            return false;
        case PrusaAccelerometer::Error::corrupted_dwarf_overflow:
            yield_error("corrupted_dwarf_overflow");
            return false;
        case PrusaAccelerometer::Error::corrupted_transmission_error:
            yield_error("corrupted_transmission_error");
            return false;
    }
    bsod("Unrecognized accelerometer error");
}

static int clear_accelerometer(PrusaAccelerometer& accelerometer) {
    PrusaAccelerometer::Acceleration dummy_sample;
    int remaining_samples = 0;
    for (int i = 0; i < 30; ++i) {
        idle(true, true);
        while(accelerometer.get_sample(dummy_sample))
            remaining_samples++;
    }
    return remaining_samples;
}

static void wait_for_movement_start(volatile phase_stepping::AxisState& axis_state) {
    move_t *last_current_movement = axis_state.current_move;
    while (last_current_movement == nullptr) {
        last_current_movement = axis_state.current_move;
        idle(false);
    }
    while (last_current_movement == axis_state.current_move) {
        idle(false);
    }
}

static void wait_for_accel_end(volatile phase_stepping::AxisState& axis_state) {
    while (axis_state.target.half_accel != 0)
        idle(false);
}

static void wait_for_zero_phase(volatile phase_stepping::AxisState& axis_state) {
    // We busy wait as the process should be fairly quick
    if (axis_state.target.start_v > 0) {
        while(phase_stepping::normalize_phase(axis_state.last_phase) <= phase_stepping::MOTOR_PERIOD / 4);
        while(phase_stepping::normalize_phase(axis_state.last_phase) >= phase_stepping::MOTOR_PERIOD / 4);
    }
    else {
        while(phase_stepping::normalize_phase(axis_state.last_phase) >= 3 * phase_stepping::MOTOR_PERIOD / 4);
        while(phase_stepping::normalize_phase(axis_state.last_phase) <= 3 * phase_stepping::MOTOR_PERIOD / 4);
    }
}

/**
 * Assuming phase stepping is enabled, measure resonance for given axis. Returns
 * measured samples via a callback, the function returns measured frequency.
 *
 * Note that the accuracy of frequency depends on the sampling time.
 */
template < typename F >
double measure_resonance(AxisEnum axis, double speed, double revs, F yield_sample) {
    static const double MOVE_MARGIN = 0.1; // To account for acceleration and deceleration we use simple hard-coded constant
    Planner::synchronize();

    volatile phase_stepping::AxisState &axis_state = phase_stepping::axis_states[axis];
    volatile phase_stepping::MoveTarget& axis_target = axis_state.target;

    // Find move target that corresponds to given number of revs
    int direction = revs > 0 ? 1 : -1;
    double axis_revs = direction * (fabs(revs) + MOVE_MARGIN);
    double a_revs = axis == AxisEnum::X_AXIS ? axis_revs : 0;
    double b_revs = axis == AxisEnum::Y_AXIS ? axis_revs : 0;

    auto [x_revs, y_revs] = physical_to_logical(a_revs, b_revs);
    auto [x_speed, y_speed] = physical_to_logical(
        axis == AxisEnum::X_AXIS ? rev_to_mm(AxisEnum::X_AXIS, speed) : 0,
        axis == AxisEnum::Y_AXIS ? rev_to_mm(AxisEnum::Y_AXIS, speed) : 0);
    double d_x = rev_to_mm(AxisEnum::X_AXIS, x_revs);
    double d_y = rev_to_mm(AxisEnum::Y_AXIS, y_revs);

    double feedrate_mms = sqrt(x_speed * x_speed + y_speed * y_speed);

    plan_move_by(feedrate_mms, d_x, d_y);

    wait_for_movement_start(axis_state);
    wait_for_accel_end(axis_state);
    wait_for_zero_phase(axis_state);

    uint32_t start_time = ticks_us();
    int samples_taken = 0;
    int counter = 0;
    PrusaAccelerometer accelerometer;
    while (axis_target.half_accel == 0) {
        counter++;
        PrusaAccelerometer::Acceleration sample;
        int new_sample = accelerometer.get_sample(sample);
        if (new_sample) {
            yield_sample(sample.val[0], sample.val[1], sample.val[2]);
            samples_taken++;
        }
    }
    uint32_t end_time = ticks_us();

    int remaining_samples = clear_accelerometer(accelerometer);
    log_debug(PhaseStepping, "%d ticks made", counter);
    log_debug(PhaseStepping, "%d samples taken", samples_taken);
    log_debug(PhaseStepping, "%d samples remaining", remaining_samples);

    // Sampling frequency
    return 1000000.0 * samples_taken / double(end_time - start_time);
}

/**
 * @brief Measure print head resonance
 *
 * Parameters:
 * - X/Y - choose motor to use
 * - F   - motion speed in rev/sec
 * - R   - number of revolutions
 *
 * Outputs raw accelerometer sample <seq>, <X>, <Y>, <Z> per line and real
 * sampling frequency of the accelerometer as "sample freq: <freq>".
 **/
void GcodeSuite::M974() {
    DisableBusyReport disable_busy_report;

    bool valid = true;

    for (char required : "FR"sv) {
        if (parser.seenval(required))
            continue;
        valid = false;
        print_error("Missing ", required, "-parameter");
    }

    int axes_count = 0;
    for ( auto [axis, letter] : SUPPORTED_AXES) {
        if (parser.seen(letter))
            axes_count++;
    }
    if (axes_count != 1) {
        print_error("Exactly one axis has to be specified");
        valid = false;
    }

    auto axis = parser.seen('X') ? AxisEnum::X_AXIS : AxisEnum::Y_AXIS;

    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis;
    if (!(PreciseStepping::physical_axis_step_generator_types & enable_mask)) {
        print_error("Phase stepping is not enabled");
        valid = false;
    }

    if (parser.floatval('F') <= 0) {
        print_error("Speed has to be positive");
        valid = false;
    }

    if (!valid) {
        print_error("Invalid parameters, no effect");
        return;
    }

    double frequency = measure_resonance(
        axis,
        parser.floatval('F'),
        parser.floatval('R'),
        [sampleNum = 0] (float x, float y, float z) mutable {
            SERIAL_ECHO(sampleNum);
            SERIAL_ECHO(", ");
            SERIAL_ECHO(x);
            SERIAL_ECHO(", ");
            SERIAL_ECHO(y);
            SERIAL_ECHO(", ");
            SERIAL_ECHOLN(z);
            sampleNum++;
        });
    SERIAL_ECHO("sample freq: ");
    SERIAL_ECHOLN(frequency);
}

/**
 * @brief Measure dwarf accelerometer sampling frequency
 *
 * Outputs sampling frequency of the accelerometer as "sample freq: <freq>".
 **/
void GcodeSuite::M975() {
    PrusaAccelerometer accelerometer;
    if (!accelerometer_ok(accelerometer, print_error))
        return;

    constexpr int request_samples_num = 3'000;

    for (int i = 0; i < request_samples_num;) {
        PrusaAccelerometer::Acceleration measured_acceleration;
        const int samples = accelerometer.get_sample(measured_acceleration);
        if (samples) {
            ++i;
        } else {
            idle(true, true);
        }
    }

    accelerometer_ok(accelerometer, print_error);
    SERIAL_ECHO("sample freq: ");
    SERIAL_ECHOLN(accelerometer.get_sampling_rate());
}
