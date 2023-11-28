#include <gcode/gcode.h>
#include <module/planner.h>
#include <module/prusa/accelerometer.h>

#include <feature/precise_stepping/precise_stepping.hpp>
#include <feature/phase_stepping/phase_stepping.hpp>
#include <feature/phase_stepping/calibration.hpp>

#include <log.h>
#include <string_view>

LOG_COMPONENT_REF(PhaseStepping);

using namespace std::literals;

static constexpr std::array<std::pair<AxisEnum, char>, 2> SUPPORTED_AXES = {
    { { X_AXIS, 'X' }, { Y_AXIS, 'Y' } }
};

// This is lambda on purpose so it can be passed around
static auto print_error = [](auto... args) {
    serial_error_start();
    (SERIAL_ECHO(args), ...);
    SERIAL_CHAR('\n');
};

static bool is_one_of(char c, std::string_view sv) {
    for (char x : sv) {
        if (c == x) {
            return true;
        }
    }
    return false;
}

static const char *fixed_repr(float number, int n = 5) {
    static char buffer[32];
    snprintf(buffer, 32, "%.*f", n, number);
    return buffer;
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
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (!parser.seen(letter)) {
            continue;
        }
        phase_stepping::enable(axis, true);
    }
}

/**
 * @brief Disable phase stepping for axis
 *
 * - valid axes X, Y
 */
void GcodeSuite::M971() {
    planner.synchronize();
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (!parser.seen(letter)) {
            continue;
        }
        phase_stepping::enable(axis, false);
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
        if (!parser.seen(letter)) {
            continue;
        }
        const phase_stepping::AxisState &axis_state = *phase_stepping::axis_states[axis];
        for (char dir : "FB"sv) {
            if (!parser.seen(letter)) {
                continue;
            }

            const auto &lut = dir == 'F'
                ? axis_state.forward_current
                : axis_state.backward_current;

            const auto &table = lut.get_correction();
            for (size_t i = 0; i != table.size(); i++) {
                SERIAL_ECHO(letter);
                SERIAL_ECHO(", ");
                SERIAL_ECHO(dir);
                SERIAL_ECHO(", ");
                SERIAL_ECHO(i);
                SERIAL_ECHO(", ");
                SERIAL_ECHO(fixed_repr(table[i].mag, 7));
                SERIAL_ECHO(", ");
                SERIAL_ECHO(fixed_repr(table[i].pha, 7));
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

        char *end_ptr;
        float first = std::strtod(str.data(), &end_ptr);
        if (end_ptr != str.data() + comma_pos) {
            print_error("Malformed input: unable to parse first value of a pair");
            return {}; // Return an empty vector on error
        }

        str.remove_prefix(comma_pos + 1);

        size_t space_pos = str.find(' ');

        float second = std::strtod(str.data(), &end_ptr);
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
    std::string_view str_arg { parser.string_arg };
    if (str_arg.size() < 3 || !is_one_of(str_arg[0], "XY") || !is_one_of(str_arg[1], "FB")) {
        print_error("Invalid format; should be <X/Y><F/B><list of mag,phase pairs separated by space");
    }

    AxisEnum axis = str_arg[0] == 'X' ? AxisEnum::X_AXIS : AxisEnum::Y_AXIS;
    phase_stepping::AxisState &axis_state = *phase_stepping::axis_states[axis];
    auto &lut = str_arg[1] == 'F'
        ? axis_state.forward_current
        : axis_state.backward_current;

    str_arg.remove_prefix(2);
    auto data = parse_pairs(str_arg);

    if (data.empty()) {
        return;
    }

    lut.modify_correction([&](auto &table) {
        for (size_t n = 0; n != table.size(); n++) {
            if (n < data.size()) {
                SERIAL_ECHO("Setting ");
                SERIAL_ECHO(n);
                SERIAL_ECHO(": ");
                SERIAL_ECHO(data[n].first);
                SERIAL_ECHO(", ");
                SERIAL_ECHOLN(data[n].second);
                table[n] = phase_stepping::SpectralItem {
                    .mag = data[n].first,
                    .pha = data[n].second
                };
            } else {
                table[n] = phase_stepping::SpectralItem {
                    .mag = 0,
                    .pha = 0
                };
            }
        }
    });
}

template <typename YieldError>
static bool accelerometer_ok(PrusaAccelerometer &acc, YieldError yield_error) {
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
    }
    bsod("Unrecognized accelerometer error");
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
        if (parser.seenval(required)) {
            continue;
        }
        valid = false;
        print_error("Missing ", required, "-parameter");
    }

    int axes_count = 0;
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (parser.seen(letter)) {
            axes_count++;
        }
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

    double frequency = phase_stepping::capture_samples(
        axis,
        parser.floatval('F'),
        parser.floatval('R'),
        [sampleNum = 0](const PrusaAccelerometer::Acceleration &sample) mutable {
            SERIAL_ECHO(sampleNum);
            SERIAL_ECHO(", ");
            SERIAL_ECHO(sample.val[0]);
            SERIAL_ECHO(", ");
            SERIAL_ECHO(sample.val[1]);
            SERIAL_ECHO(", ");
            SERIAL_ECHOLN(sample.val[2]);
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
    if (!accelerometer_ok(accelerometer, print_error)) {
        return;
    }

    constexpr int request_samples_num = 3'000;

    auto report = [sampleNum = 0](const PrusaAccelerometer::Acceleration &sample) mutable {
        SERIAL_ECHO(sampleNum);
        SERIAL_ECHO(", ");
        SERIAL_ECHO(sample.val[0]);
        SERIAL_ECHO(", ");
        SERIAL_ECHO(sample.val[1]);
        SERIAL_ECHO(", ");
        SERIAL_ECHOLN(sample.val[2]);
        sampleNum++;
    };

    for (int i = 0; i < request_samples_num;) {
        PrusaAccelerometer::Acceleration measured_acceleration;
        const int samples = accelerometer.get_sample(measured_acceleration);
        if (samples) {
            ++i;
            report(measured_acceleration);
        } else {
            idle(true, true);
        }
    }

    accelerometer_ok(accelerometer, print_error);
    SERIAL_ECHO("sample freq: ");
    SERIAL_ECHOLN(accelerometer.get_sampling_rate());
}

/**
 * @brief Measure print head resonance
 *
 * Parameters:
 * - X/Y - choose motor to use
 * - F   - motion speed in rev/sec
 * - R   - number of revolutions
 *
 * Outputs frequency response
 **/
void GcodeSuite::M976() {
    DisableBusyReport disable_busy_report;

    bool valid = true;

    for (char required : "FR"sv) {
        if (parser.seenval(required)) {
            continue;
        }
        valid = false;
        print_error("Missing ", required, "-parameter");
    }

    int axes_count = 0;
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (parser.seen(letter)) {
            axes_count++;
        }
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

    auto analysis = phase_stepping::analyze_resonance(
        axis, parser.floatval('F'), parser.floatval('R'),
        { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } });

    if (!analysis.has_value()) {
        print_error("Unsuccessful data capture");
    }

    auto &data = *analysis;

    for (std::size_t i = 0; i != data.size(); i++) {
        SERIAL_ECHO(i);
        SERIAL_ECHO(": ");
        SERIAL_ECHOLN(data[i]);
    }
}

class GCodeCalibrationReporter : public phase_stepping::CalibrationReporterBase {
public:
    void on_initial_movement() override {
        SERIAL_ECHOLN("Moving to calibration position");
    }

    void on_calibration_phase_progress(int progress) override {
        SERIAL_ECHO("Phase ");
        SERIAL_ECHO(_current_calibration_phase + 1);
        SERIAL_ECHO("/");
        SERIAL_ECHO(_calibration_phases_count);
        SERIAL_ECHO(": ");
        SERIAL_ECHO(progress);
        SERIAL_ECHOLN("%");
    }

    void on_calibration_phase_result(float forward_score, float backward_score) override {
        SERIAL_ECHO("Phase ");
        SERIAL_ECHO(_current_calibration_phase + 1);
        SERIAL_ECHO(" done. Vibration reduced by: ");
        SERIAL_ECHO(100.f - forward_score * 100.f);
        SERIAL_ECHO("%, ");
        SERIAL_ECHO(100.f - backward_score * 100.f);
        SERIAL_ECHO("%\n");
    };

    void on_termination() override {
        SERIAL_ECHOLN("Done");
    }
};

/**
 * @brief Calibrate motor
 *
 * Parameters:
 * - X/Y - choose motor to use
 *
 * Calibrates given motor and sets the newly found compensation.
 **/
void GcodeSuite::M977() {
    DisableBusyReport _;

    bool valid = true;

    int axes_count = 0;
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (parser.seen(letter)) {
            axes_count++;
        }
    }
    if (axes_count != 1) {
        print_error("Exactly one axis has to be specified");
        valid = false;
    }

    auto axis = parser.seen('X') ? AxisEnum::X_AXIS : AxisEnum::Y_AXIS;

    if (!valid) {
        print_error("Invalid parameters, no effect");
        return;
    }

    SERIAL_ECHO("Axis: ");
    SERIAL_ECHOLN(axis);

    G28_no_parser( // home
        true, // always_home_all
        true, // home only if needed,
        3, // raise Z by 3 mm
        false, // S-parameter,
        true, true, false // home X, Y but not Z
    );
    Planner::synchronize();

    GCodeCalibrationReporter reporter;
    auto result = phase_stepping::calibrate_axis(axis, reporter);

    if (!result.has_value()) {
        print_error("Calibration failed");
        return;
    }

    auto [forward, backward] = *result;

    for (int i = 0; i != phase_stepping::CORRECTION_HARMONICS; i++) {
        SERIAL_ECHO(i);
        SERIAL_ECHO(": F");
        SERIAL_ECHO(fixed_repr(forward[i].mag));
        SERIAL_ECHO(",");
        SERIAL_ECHO(fixed_repr(forward[i].pha));
        SERIAL_ECHO(" B");
        SERIAL_ECHO(fixed_repr(backward[i].mag));
        SERIAL_ECHO(",");
        SERIAL_ECHO(fixed_repr(backward[i].pha));
        SERIAL_ECHO("\n");
    }
}
