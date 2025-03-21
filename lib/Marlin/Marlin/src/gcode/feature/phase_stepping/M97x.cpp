#include <gcode/gcode.h>
#include <gcode/gcode_parser.hpp>
#include <module/planner.h>
#include <module/prusa/accelerometer.h>

#include <feature/precise_stepping/precise_stepping.hpp>
#include <feature/phase_stepping/phase_stepping.hpp>
#include <feature/phase_stepping/calibration.hpp>

#include <vector>
#include <string_view>
#include <config_store/store_instance.hpp>

#include <USBSerial.h>

using namespace std::literals;
using phase_stepping::opts::SERIAL_DECIMALS;

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

void M970_report(bool eeprom = false) {
    SERIAL_ECHO("M970");
    for (auto [axis, letter] : SUPPORTED_AXES) {
        SERIAL_ECHOPAIR(" ", letter);
        bool state = eeprom ? config_store().get_phase_stepping_enabled(axis)
                            : phase_stepping::is_enabled(axis);
        SERIAL_ECHO(state ? "1" : "0");
    }
    SERIAL_ECHOLN();
}

static void report_state() {
#if HAS_BURST_STEPPING()
    SERIAL_ECHO("phstep (burst)");
#else
    SERIAL_ECHO("phstep");
#endif
    if (phase_stepping::any_axis_enabled()) {
        SERIAL_ECHOLN(": active");
    } else {
        SERIAL_ECHOLN(": disabled");
    }
    M970_report();
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M970: Enable/Disable Phase Stepping <a href="https://reprap.org/wiki/G-code#M970:_Enable.2FDisable_Phase_Stepping">M970: Enable/Disable Phase Stepping</a>
 *
 * Only XL and iX
 *
 *#### Usage
 *
 *    M970 [ X | Y ]
 *
 *#### Parameters
 *
 * - `X` - Phase steppring for X motor driver
 * - `Y` - Phase steppring for Y motor driver
 *
 * Without parameters prints the current state of the phase stepping
 */
void GcodeSuite::M970() {
    planner.synchronize();
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (!parser.seen(letter)) {
            continue;
        }
        bool enabled = parser.value_bool();
        phase_stepping::enable(axis, enabled);
        config_store().set_phase_stepping_enabled(axis, enabled);
    }
    report_state();
}

struct OptionsM971 {
    const bool axis_x : 1;
    const bool axis_y : 1;
    const bool forward : 1;
    const bool backward : 1;
    const bool reset : 1;
    const std::optional<int> index;
    const std::optional<float> magnitude;
    const std::optional<float> phase;
};

static void M971_error_enabled() {
    SERIAL_ERROR_MSG("Phase-stepping must be disabled to modify its parameters.");
}
static void M971_error_syntax() {
    SERIAL_ERROR_MSG("Syntax.");
}

static void M971_reset_axis(const OptionsM971 &options, AxisEnum axis) {
    assert(!phase_stepping::is_enabled(axis));
    if (options.forward) {
        phase_stepping::remove_from_persistent_storage(axis, phase_stepping::CorrectionType::forward);
    }
    if (options.backward) {
        phase_stepping::remove_from_persistent_storage(axis, phase_stepping::CorrectionType::backward);
    }
}

static void M971_reset(const OptionsM971 &options) {
    // First, check if operation can be carried away.
    if (options.axis_x && phase_stepping::is_enabled(X_AXIS)) {
        return M971_error_enabled();
    }
    if (options.axis_y && phase_stepping::is_enabled(Y_AXIS)) {
        return M971_error_enabled();
    }

    // This can't fail now.
    if (options.axis_x) {
        M971_reset_axis(options, X_AXIS);
    }
    if (options.axis_y) {
        M971_reset_axis(options, Y_AXIS);
    }
}

static void M971_read_axis_direction(const phase_stepping::CorrectedCurrentLut &lut, char axis_letter, char direction_letter) {
    const auto &table = lut.get_correction();
    for (size_t index = 0; index != table.size(); index++) {
        const float magnitude = table[index].mag;
        const float phase = table[index].pha;

        SERIAL_ECHO("M971 ");
        SERIAL_ECHO(axis_letter);
        SERIAL_ECHO(" ");
        SERIAL_ECHO(direction_letter);
        SERIAL_ECHO(" I");
        SERIAL_ECHO(index);
        SERIAL_ECHO(" M");
        SERIAL_PRINT(magnitude, SERIAL_DECIMALS);
        SERIAL_ECHO(" P");
        SERIAL_PRINTLN(phase, SERIAL_DECIMALS);
    }
}

static void M971_read_axis(const OptionsM971 &options, const phase_stepping::AxisState &axis_state, char axis) {
    if (options.forward) {
        M971_read_axis_direction(axis_state.forward_current, axis, 'F');
    }
    if (options.backward) {
        M971_read_axis_direction(axis_state.backward_current, axis, 'B');
    }
}

static void M971_read(const OptionsM971 &options) {
    if (options.axis_x) {
        M971_read_axis(options, phase_stepping::axis_states[X_AXIS], 'X');
    }
    if (options.axis_y) {
        M971_read_axis(options, phase_stepping::axis_states[Y_AXIS], 'Y');
    }
}

static void M971_write_axis_direction(const OptionsM971 &options, phase_stepping::CorrectedCurrentLut &lut, const char *path) {
    // All three options are required
    if (!options.index || !options.magnitude || !options.phase) {
        return M971_error_syntax();
    }
    const auto index = *options.index;
    const auto magnitude = *options.magnitude;
    const auto phase = *options.phase;

    lut.modify_correction([&](phase_stepping::MotorPhaseCorrection &table) {
        if (0 <= index && index < (int)table.size()) {
            table[index] = phase_stepping::SpectralItem { .mag = magnitude, .pha = phase };
        } else {
            return M971_error_syntax();
        }
    });

    phase_stepping::save_correction_to_file(lut, path);
}

static void M971_write_axis(const OptionsM971 &options, AxisEnum axis) {
    // writing while phase-stepping is enabled is not supported
    if (phase_stepping::is_enabled(axis)) {
        return M971_error_enabled();
    }
    // directions are mutually exclusive when writing
    if (options.forward && options.backward) {
        return M971_error_syntax();
    } else if (options.forward) {
        M971_write_axis_direction(
            options,
            phase_stepping::axis_states[axis].forward_current,
            phase_stepping::get_correction_file_path(axis, phase_stepping::CorrectionType::forward));
    } else if (options.backward) {
        M971_write_axis_direction(
            options,
            phase_stepping::axis_states[axis].backward_current,
            phase_stepping::get_correction_file_path(axis, phase_stepping::CorrectionType::backward));
    } else {
        return M971_error_syntax();
    }
}

static void M971_write(const OptionsM971 &options) {
    // axes are mutually exclusive when writing
    if (options.axis_x && options.axis_y) {
        return M971_error_syntax();
    } else if (options.axis_x) {
        return M971_write_axis(options, X_AXIS);
    } else if (options.axis_y) {
        return M971_write_axis(options, Y_AXIS);
    } else {
        return M971_error_syntax();
    }
}

/**
 *### M971: Read/reset/write phase-stepping motor current correction
 *
 * Only XL/iX/COREONE
 *
 *#### Usage
 *
 *    M971 [X] [Y] [F] [B]
 *    M971 [X] [Y] [F] [B] R
 *    M971 X|Y F|B I<index> M<magnitude> P<phase>
 *
 *#### Parameters
 *
 * - `X` - X axis
 * - `Y` - Y axis
 * -
 * - `F` - Forward direction
 * - `B` - Backward direction
 * - `R` - Reset corrections
 * - `I` - Index in the table, starting at 0
 * - `M` - Magnitude of the motor correction
 * - `P` - Phase of the motor correction
 **/
void GcodeSuite::M971() {
    GCodeParser2 parser2;
    if (!parser2.parse_marlin_command()) {
        return M971_error_syntax();
    }
    const OptionsM971 options {
        .axis_x = parser2.option<bool>('X').value_or(false),
        .axis_y = parser2.option<bool>('Y').value_or(false),
        .forward = parser2.option<bool>('F').value_or(false),
        .backward = parser2.option<bool>('B').value_or(false),
        .reset = parser2.option<bool>('R').value_or(false),
        .index = parser2.option<int>('I'),
        .magnitude = parser2.option<float>('M'),
        .phase = parser2.option<float>('P'),
    };
    if (options.reset) {
        return M971_reset(options);
    } else if (parser.seen('I') && parser.seen('M') && parser.seen('P')) {
        return M971_write(options);
    } else {
        return M971_read(options);
    }
}

/**
 *### M972: Retrieve current correction <a href="https://reprap.org/wiki/G-code#M972:_Retrieve_Current_Correction">M972: Retrieve Current Correction</a>
 *
 * Only XL and iX
 *
 *#### Usage
 *
 *    M972 [ X | Y ]
 *
 * Outputs the current correction table in format: `<axis>, <direction>, <index>, <mag>, <pha>` per line. Multiple axes can be specified.
 **/
void GcodeSuite::M972() {
    for (auto [axis, letter] : SUPPORTED_AXES) {
        if (!parser.seen(letter)) {
            continue;
        }
        const phase_stepping::AxisState &axis_state = phase_stepping::axis_states[axis];
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
                SERIAL_PRINT(table[i].mag, SERIAL_DECIMALS);
                SERIAL_ECHO(", ");
                SERIAL_PRINT(table[i].pha, SERIAL_DECIMALS);
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
 *### M973: Set single entry for the current correction table <a href=" "> </a>
 *
 * Only XL and iX
 *
 *#### Usage
 *
 *    M973 [ X | Y | F | B ]
 *
 *#### Parameters
 *
 * - `X` - X axis
 * - `Y` - Y axis
 * - `F` - Forward direction
 * - `B` - Backward direction
 * - `<mag,phase>` - mag and phase
 *
 * String argument in format: <X/Y><F/B><list of mag,phase pairs separated by space>
 **/
void GcodeSuite::M973() {
    std::string_view str_arg { parser.string_arg };
    if (str_arg.size() < 3 || !is_one_of(str_arg[0], "XY") || !is_one_of(str_arg[1], "FB")) {
        print_error("Invalid format; should be <X/Y><F/B><list of mag,phase pairs separated by space");
    }

    AxisEnum axis = str_arg[0] == 'X' ? AxisEnum::X_AXIS : AxisEnum::Y_AXIS;
    phase_stepping::AxisState &axis_state = phase_stepping::axis_states[axis];
    const bool forward_correction { str_arg[1] == 'F' };
    auto &lut = forward_correction
        ? axis_state.forward_current
        : axis_state.backward_current;

    str_arg.remove_prefix(2);
    auto data = parse_pairs(str_arg);

    if (data.size() && data.size() != lut.get_correction().size()) {
        print_error("Invalid count (", data.size(), ") of <mag,phase> entries. Expected ", lut.get_correction().size());
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

    phase_stepping::save_correction_to_file(lut, phase_stepping::get_correction_file_path(axis, forward_correction ? phase_stepping::CorrectionType::forward : phase_stepping::CorrectionType::backward));
}

/**
 *### M975: Measure dwarf accelerometer sampling frequency <a href="https://reprap.org/wiki/G-code#M975:_Measure_Dwarf_Accelerometer_Sampling_Frequency">M975: Measure Dwarf Accelerometer Sampling Frequency</a>
 *
 * Only XL and iX
 *
 *#### Usage
 *
 *    M975
 *
 * Outputs sampling frequency of the accelerometer as "sample freq: <freq>".
 **/
void GcodeSuite::M975() {
    PrusaAccelerometer accelerometer;
    if (accelerometer.report_error(print_error)) {
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

    accelerometer.clear();

    for (int i = 0; i < request_samples_num;) {
        PrusaAccelerometer::Acceleration measured_acceleration;
        using GetSampleResult = PrusaAccelerometer::GetSampleResult;
        const GetSampleResult get_sample_result = accelerometer.get_sample(measured_acceleration);

        switch (get_sample_result) {

        case GetSampleResult::ok:
            ++i;
            report(measured_acceleration);
            break;

        case GetSampleResult::buffer_empty:
            idle(true, true);
            break;

        case GetSampleResult::error:
            accelerometer.report_error(print_error);
            return;
        }
    }

    SERIAL_ECHO("sample freq: ");
    SERIAL_ECHOLN(accelerometer.get_sampling_rate());
}

/**
 *### M976: Measure print head resonance <a href="https://reprap.org/wiki/G-code#M976:_Measure_Print_Head_Resonance">M976: Measure Print Head Resonance</a>
 *
 * Only XL and iX
 *
 *#### Usage
 *
 *    M976 [ X | Y | F | R ]
 *
 *#### Parameters
 *
 * - `X` - X motor
 * - `Y` - Y motor
 * - `F` - motion speed in rev/sec
 * - `R` - number of revolutions
 *
 * Outputs frequency response
 **/
void GcodeSuite::M976() {
    TEMPORARY_AUTO_REPORT_OFF(suspend_auto_report);

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

    if (analysis.empty()) {
        print_error("Unsuccessful data capture");
    }

    for (std::size_t i = 0; i != analysis.size(); i++) {
        SERIAL_ECHO(i);
        SERIAL_ECHO(": ");
        SERIAL_ECHOLN(analysis[i]);
    }
}

class CalibrateAxisHooks final : public phase_stepping::CalibrateAxisHooks {
    std::vector<std::tuple<float, float>> _calibration_results;
    std::size_t _current_calibration_phase = 0;

public:
    void on_motor_characterization_start() override {
        SERIAL_ECHO("Characterizing motor... ");
    }

    virtual void on_motor_characterization_result(int phases) override {
        SERIAL_ECHO(" done. ");
        SERIAL_ECHO(phases);
        SERIAL_ECHOLN(" phases to follow");
        _calibration_results.resize(phases);
    }

    void on_enter_calibration_phase(int calibration_phase) override {
        _current_calibration_phase = calibration_phase;
        SERIAL_ECHO("Entering phase ");
        SERIAL_ECHOLN(calibration_phase + 1);
    }

    void on_calibration_phase_result(float forward_score, float backward_score) override {
        assert(_current_calibration_phase < _calibration_results.size());

        _calibration_results[_current_calibration_phase] = { forward_score, backward_score };
        SERIAL_ECHO("Phase ");
        SERIAL_ECHO(_current_calibration_phase + 1);
        SERIAL_ECHO(" done. Vibration reduced by: ");
        SERIAL_ECHO(100.f - forward_score * 100.f);
        SERIAL_ECHO("%, ");
        SERIAL_ECHO(100.f - backward_score * 100.f);
        SERIAL_ECHO("%\n");
    };

    void on_termination() override {
        SERIAL_ECHOLN("Calibration done");
    }

    ContinueOrAbort on_idle() override {
        return ContinueOrAbort::Continue;
    }
};

/**
 *### M977: Calibrate motor <a href="https://reprap.org/wiki/G-code#M977:_Calibrate_Motor">M977: Calibrate Motor</a>
 *
 * Only XL and iX
 *
 *#### Usage
 *
 *    M977 [ X | Y ]
 *
 *#### Parameters
 *
 * - `X` - X motor
 * - `Y` - Y motor
 *
 * Calibrates given motor and sets the newly found compensation.
 **/
void GcodeSuite::M977() {
    TEMPORARY_AUTO_REPORT_OFF(suspend_auto_report);

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

    G28_no_parser(true, true, false, { .only_if_needed = true, .z_raise = 3 });
    do_blocking_move_to_z(50);
    Planner::synchronize();

    CalibrateAxisHooks hooks;
    auto result = phase_stepping::calibrate_axis(axis, hooks);

    if (!result.has_value()) {
        SERIAL_ECHO(result.error());
        print_error("Calibration failed");
        return;
    }

    auto [forward, backward] = *result;

    for (size_t i = 0; i != forward.size(); i++) {
        SERIAL_ECHO(i);
        SERIAL_ECHO(": F");
        SERIAL_PRINT(forward[i].mag, SERIAL_DECIMALS);
        SERIAL_ECHO(",");
        SERIAL_PRINT(forward[i].pha, SERIAL_DECIMALS);
        SERIAL_ECHO(" B");
        SERIAL_PRINT(backward[i].mag, SERIAL_DECIMALS);
        SERIAL_ECHO(",");
        SERIAL_PRINT(backward[i].pha, SERIAL_DECIMALS);
        SERIAL_ECHO("\n");
    }
    phase_stepping::save_to_persistent_storage_without_enabling(axis);
}

static void dump_samples_annotation(const phase_stepping::SamplesAnnotation ant) {
    SERIAL_ECHO("sampling_freq: ");
    SERIAL_ECHOLN(ant.sampling_freq);
    SERIAL_ECHO("movement_ok: ");
    SERIAL_ECHOLN(ant.movement_ok ? "true" : "false");
    SERIAL_ECHO("accel_error: ");
    SERIAL_ECHOLN(int(ant.accel_error));
    SERIAL_ECHO("start_marker: ");
    SERIAL_ECHOLN(ant.start_marker);
    SERIAL_ECHO("end_marker: ");
    SERIAL_ECHOLN(ant.end_marker);
    SERIAL_ECHO("signal_start: ");
    SERIAL_ECHOLN(ant.signal_start);
    SERIAL_ECHO("signal_end: ");
    SERIAL_ECHOLN(ant.signal_end);
}

/**
 * @brief G-code M978: Perform phase and magnitude correction sweep.
 *
 * This G-code command performs a phase and magnitude correction sweep by
 * adjusting the phase and magnitude correction parameters over a specified
 * range. The harmonic on which to perform the sweep is also specified.
 *
 * Parameters:
 * - A<float>: Start phase correction in radians.
 * - B<float>: End phase correction in radians.
 * - C<float>: Start magnitude correction (must be in the range -1 to 1).
 * - D<float>: End magnitude correction (must be in the range -1 to 1).
 * - H<int>: Harmonic to perform the sweep on (must be in the range 1 to 16).
 *
 * The function will output raw accelerometer samples to the serial port in the
 * format: `<index>: <sample>`.
 *
 * Metadata of the movement are returned as `<key>: <value>` pairs.
 *
 * Error conditions:
 * - If the harmonic parameter is not in the range 1 to 16, an error is reported.
 * - If the magnitude correction parameters are not in the range -1 to 1, an error is reported.
 *
 * If any error condition is met, the function will report the error and return
 * without performing the sweep.
 */
void GcodeSuite::M978() {
    TEMPORARY_AUTO_REPORT_OFF(suspend_auto_report);

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

    phase_stepping::SweepParams params;
    params.pha_start = parser.seenval('A') ? parser.floatval('A') : 0;
    params.pha_end = parser.seenval('B') ? parser.floatval('B') : 0;
    params.mag_start = parser.seenval('C') ? parser.floatval('C') : 0;
    params.mag_end = parser.seenval('D') ? parser.floatval('D') : 0;

    int harmonic = parser.seenval('H') ? parser.intval('H') : 0;
    float revs = parser.seenval('R') ? parser.floatval('R') : 0;
    float speed = parser.seenval('F') ? parser.floatval('F') : 0;

    if (harmonic < 1 || harmonic > 16) {
        print_error("Harmonic must be in the range 1 to 16");
        valid = false;
    }

    if (params.mag_start < -1 || params.mag_start > 1 || params.mag_end < -1 || params.mag_end > 1) {
        print_error("Magnitude correction must be in the range -1 to 1");
        valid = false;
    }

    if (speed <= 0) {
        print_error("Speed must be positive");
        valid = false;
    }

    if (!valid) {
        print_error("Invalid parameters, no effect");
        return;
    }

    auto result = phase_stepping::capture_param_sweep_samples(
        axis,
        speed,
        revs,
        harmonic,
        params,
        [n = 0](float sample) mutable {
            char buff[64];
            snprintf(buff, sizeof(buff), "%d, %.5f\n", n++, sample);
            int len = strlen(buff);
            SerialUSB.cdc_write_sync(reinterpret_cast<uint8_t *>(buff), len);
        });
    dump_samples_annotation(result);
}

/**
 * @brief G-code M979: Perform motor resonance measurement during a speed sweep.
 *
 * This G-code command performs a motor resonance measurement by sweeping the
 * motor speed from a specified start speed to an end speed over a given number
 * of revolutions. The direction of movement is determined by the sign of the
 * revolutions parameter.
 *
 * Parameters:
 * - A<float>: Start speed in revolutions per second (must be non-negative).
 * - B<float>: End speed in revolutions per second (must be non-negative).
 * - R<float>: Number of revolutions for the sweep (must be non-zero, sign
 *   determines direction).
 * - X, Y: Axis to perform the sweep on (exactly one axis must be
 *   specified).
 *
 * The function will output raw accelerometer samples to the serial port in the
 * format: `<index>: <sample>`.
 *
 * Metadata of the movement are returned as `<key>: <value>` pairs.
 *
 * Error conditions:
 * - If no axis or more than one axis is specified, an error is reported.
 * - If both start and end speeds are zero, an error is reported.
 * - If any speed is negative, an error is reported.
 * - If the revolutions parameter is zero, an error is reported.
 *
 * If any error condition is met, the function will report the error and return
 * without performing the sweep.
 */
void GcodeSuite::M979() {
    TEMPORARY_AUTO_REPORT_OFF(suspend_auto_report);

    bool valid = true;

    int axes_count = 0;
    AxisEnum axis = AxisEnum::X_AXIS; // Default initialization
    for (auto [ax, letter] : SUPPORTED_AXES) {
        if (parser.seen(letter)) {
            axis = ax;
            axes_count++;
        }
    }
    if (axes_count != 1) {
        print_error("Exactly one axis has to be specified");
        valid = false;
    }

    float start_speed = parser.seenval('A') ? parser.floatval('A') : 0;
    float end_speed = parser.seenval('B') ? parser.floatval('B') : 0;
    float revs = parser.seenval('R') ? parser.floatval('R') : 0;

    if (start_speed < 0 || end_speed < 0) {
        print_error("Speeds must be positive");
        valid = false;
    }

    if (start_speed == 0 && end_speed == 0) {
        print_error("At least one speed must be non-zero");
        valid = false;
    }

    if (revs == 0) {
        print_error("Revolutions (R) parameter must be non-zero");
        valid = false;
    }

    if (!valid) {
        print_error("Invalid parameters, no effect");
        return;
    }

    auto result = phase_stepping::capture_speed_sweep_samples(
        axis,
        start_speed,
        end_speed,
        revs,
        [n = 0](float sample) mutable {
            char buff[64];
            snprintf(buff, sizeof(buff), "%d, %.5f\n", n++, sample);
            int len = strlen(buff);
            SerialUSB.cdc_write_sync(reinterpret_cast<uint8_t *>(buff), len);
        });
    dump_samples_annotation(result);
}

/** @}*/
