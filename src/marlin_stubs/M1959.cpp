#include "M1959.hpp"

#include <buddy/unreachable.hpp>
#include <common/marlin_server.hpp>
#include <common/str_utils.hpp>
#include <Marlin/src/gcode/calibrate/M958.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/tool_change.h>
#include <option/development_items.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_attachable_accelerometer.h>
#include <option/has_remote_accelerometer.h>

static_assert(HAS_INPUT_SHAPER_CALIBRATION());

LOG_COMPONENT_REF(Marlin);

static void log_axis_config(const input_shaper::AxisConfig &axis_config, char axis) {
    log_info(Marlin, "axis=%c type=%u frequency=%f damping_ratio=%f vibration_reduction=%f",
        axis,
        static_cast<int>(axis_config.type),
        static_cast<double>(axis_config.frequency),
        static_cast<double>(axis_config.damping_ratio),
        static_cast<double>(axis_config.vibration_reduction));
}

static constexpr bool klipper_mode = true;
static constexpr float acceleration_requested = 2.5f;
static constexpr uint32_t cycles = 50;

static void set_test_result(TestResult test_result) {
    config_store().selftest_result_input_shaper_calibration.set(test_result);
}

using marlin_server::wait_for_response;

static bool was_abort_requested(PhasesInputShaperCalibration phase) {
    switch (marlin_server::get_response_from_phase(phase)) {
    case Response::Abort:
        return true;
    case Response::_none:
        return false;
    default:
        std::terminate();
    }
}

struct FrequencyRange {
    int start;
    int end;
    int increment;
};

constexpr FrequencyRange frequency_range {
    .start = 30,
    .end = 80,
    .increment = 1,
};

static constexpr size_t sample_count(FrequencyRange frequency_range) {
    return (frequency_range.end - frequency_range.start) / frequency_range.increment;
}

class FrequencyRangeSpectrum final : public Spectrum {
public:
    float max() const final {
        return *std::max_element(samples.begin(), samples.end());
    }
    size_t size() const final {
        return samples.size();
    }
    FrequencyGain get(size_t index) const final {
        return {
            .frequency = static_cast<float>(frequency_range.start + index * frequency_range.increment),
            .gain = samples[index],
        };
    }

    bool is_valid() const {
        // Not sure about the physical interpretation of this heuristic but it seems to work...
        return std::all_of(samples.begin(), samples.end(), [](float sample) {
            return sample > 0.001f;
        });
    }

    void dump(char axis) const {
#if DEVELOPMENT_ITEMS()
        // I don't really care about timezones, efficiency and errors since this is debug-only...
        std::array<char, 1 + strlen("YYYYmmddHHMMSS")> strftime_buffer;
        const time_t curr_sec = time(nullptr);
        struct tm now;
        localtime_r(&curr_sec, &now);
        (void)strftime(strftime_buffer.data(), strftime_buffer.size(), "%Y%m%d%H%M%S", &now);
        std::array<char, 50> path;
        snprintf(path.data(), path.size(), "/usb/spectrum_%c_%s.txt", axis, strftime_buffer.data());

        FILE *f = fopen(path.data(), "w");
        if (f) {
            std::array<char, 32> buffer;
            for (size_t i = 0; i < size(); ++i) {
                FrequencyGain fg = get(i);
                size_t n = snprintf(buffer.data(), buffer.size(), "%f\t%f\n", (double)fg.frequency, (double)fg.gain);
                fwrite(buffer.data(), n, 1, f);
            }
            fflush(f);
            fclose(f);
        }
#else
        std::ignore = axis;
#endif
    }

    std::array<float, sample_count(frequency_range)> samples;
};

struct Context {
    FrequencyRangeSpectrum spectrum_x;
    FrequencyRangeSpectrum spectrum_y;
    input_shaper::AxisConfig axis_config_x;
    input_shaper::AxisConfig axis_config_y;
#if HAS_PHASE_STEPPING()
    phase_stepping::StateRestorer phstep_restorer;
#endif
};

static PhasesInputShaperCalibration info_proceed() {
#if HAS_ATTACHABLE_ACCELEROMETER()
    // Check the accelerometer now. It would be annoying to do all the homing
    // and parking moves and then tell the user to turn off the printer, just
    // to do all the moves after the reboot again.
    PrusaAccelerometer accelerometer;
    if (PrusaAccelerometer::Error::none == accelerometer.get_error()) {
        return PhasesInputShaperCalibration::parking;
    }
    return PhasesInputShaperCalibration::connect_to_board;
#else
    // Proceed straight to parking stage, any accelerometer error will be reported
    // after it happens.
    return PhasesInputShaperCalibration::parking;
#endif
}

static PhasesInputShaperCalibration info() {
#if PRINTER_IS_PRUSA_XL()
    // On XL, we don't need to be shown the info that asks the user to "ensure that the accelerometer is installed" - it's always on the board
    return info_proceed();

#else
    switch (wait_for_response(PhasesInputShaperCalibration::info)) {
    case Response::Continue:
        return info_proceed();
    case Response::Abort:
        // do not set_test_result()
        return PhasesInputShaperCalibration::finish;
    default:
        std::terminate();
    }
#endif
}

// Note: This is only relevant for printers which HAS_ATTACHABLE_ACCELEROMETER()
//       which coincidentally only have one hotend.
static constexpr uint8_t hotend = 0;
static constexpr float safe_temperature = 50;

static PhasesInputShaperCalibration parking(Context &context) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::parking);

#if HAS_ATTACHABLE_ACCELEROMETER()
    // Start cooling the hotend even before parking to save some time
    Temperature::disable_hotend();
    if (Temperature::degHotend(hotend) > safe_temperature) {
        Temperature::set_fan_speed(0, 255);
    }
#endif

    // Home if not homed
    if (!all_axes_known()) {
        GcodeSuite::G28_no_parser(true, true, true, { .only_if_needed = true, .z_raise = 0 });
    }

#if HAS_REMOTE_ACCELEROMETER()
    // Without tool being picked there is no accelerometer data
    if (prusa_toolchanger.has_tool() == false) {
        tool_change(/*tool_index=*/0, tool_return_t::no_return, tool_change_lift_t::no_lift, /*z_down=*/false);
    }
#endif

    // Ensure consistent measurement
    const xyz_pos_t pos = { X_BED_SIZE / 2, Y_BED_SIZE / 2, Z_SIZE / 2 };
    plan_park_move_to_xyz(pos, HOMING_FEEDRATE_XY, HOMING_FEEDRATE_Z, /*segmented=*/false);

#if HAS_PHASE_STEPPING()
    // Ensure phase stepping is disabled throughout the calibration as we manipulate steps directly
    context.phstep_restorer.set_state(false);
#else
    std::ignore = context;
#endif

    // Carry on the changes
    planner.synchronize();
#if HAS_ATTACHABLE_ACCELEROMETER()
    return PhasesInputShaperCalibration::wait_for_extruder_temperature;
#else
    return PhasesInputShaperCalibration::measuring_x_axis;
#endif
}

#if HAS_ATTACHABLE_ACCELEROMETER()

static PhasesInputShaperCalibration connect_to_board(Context &) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::connect_to_board);
    switch (wait_for_response(PhasesInputShaperCalibration::connect_to_board)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    default:
        break;
    }
    BUDDY_UNREACHABLE();
}

static PhasesInputShaperCalibration wait_for_extruder_temperature(Context &) {
    for (;;) {
        switch (marlin_server::get_response_from_phase(PhasesInputShaperCalibration::wait_for_extruder_temperature)) {
        case Response::Abort:
            return PhasesInputShaperCalibration::finish;
        case Response::_none:
            if (const float temperature = Temperature::degHotend(hotend); temperature > safe_temperature) {
                const uint16_t uint16_temperature = temperature;
                const fsm::PhaseData data = {
                    static_cast<uint8_t>((uint16_temperature >> 8) & 0xff),
                    static_cast<uint8_t>((uint16_temperature >> 0) & 0xff),
                    0,
                    0,
                };
                marlin_server::fsm_change(PhasesInputShaperCalibration::wait_for_extruder_temperature, data);
                idle(true);
            } else {
                Temperature::zero_fan_speeds();
                return PhasesInputShaperCalibration::attach_to_extruder;
            }
            break;
        default:
            BUDDY_UNREACHABLE();
        }
    }
    BUDDY_UNREACHABLE();
}

static PhasesInputShaperCalibration attach_to_extruder(Context &) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::attach_to_extruder);
    switch (wait_for_response(PhasesInputShaperCalibration::attach_to_extruder)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    case Response::Continue:
        return PhasesInputShaperCalibration::measuring_x_axis;
    default:
        break;
    }
    BUDDY_UNREACHABLE();
}

static PhasesInputShaperCalibration attach_to_bed(Context &) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::attach_to_bed);
    switch (wait_for_response(PhasesInputShaperCalibration::attach_to_bed)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    case Response::Continue:
        return PhasesInputShaperCalibration::measuring_y_axis;
    default:
        break;
    }
    BUDDY_UNREACHABLE();
}

#endif

// helper
static PhasesInputShaperCalibration measuring_axis(
    const PhasesInputShaperCalibration phase,
    const PhasesInputShaperCalibration next_phase,
    const AxisEnum logicalAxis,
    const StepEventFlag_t axis_flag,
    FrequencyRangeSpectrum &spectrum) {

    fsm::PhaseData data {
        static_cast<uint8_t>(frequency_range.start),
        static_cast<uint8_t>(frequency_range.end),
        static_cast<uint8_t>(0), // current progress
        1, // data[3] == 1 calibrating
    };
    marlin_server::fsm_change(phase, data);

    // phstep needs to be off _before_ getting the current ustep resolution
    phase_stepping::EnsureDisabled phaseSteppingDisabler;
    MicrostepRestorer microstepRestorer;
    enable_all_steppers(); // enable all axes to have the same state as printing

    stepper_microsteps(logicalAxis, 128);

    VibrateMeasureParams args {
        .excitation_acceleration = acceleration_requested,
        .excitation_cycles = cycles,
        .klipper_mode = klipper_mode,
        .calibrate_accelerometer = true,
        .axis_flag = axis_flag,
    };
    if (!args.setup(microstepRestorer)) {
        bsod("setup failed");
    }

    float frequency = frequency_range.start;

    struct {
        bool aborted = false;
        float prev_progress = -1;
        PhasesInputShaperCalibration phase;

    } progress_hook_data {
        .phase = phase
    };
    const auto progress_hook = [&progress_hook_data](const VibrateMeasureProgressHookParams &params) {
        progress_hook_data.aborted |= was_abort_requested(progress_hook_data.phase);
        if (progress_hook_data.aborted) {
            return false;
        }

        // data[3] == 1 calibrating
        if (params.phase == VibrateMeasureProgressHookParams::Phase::calibrating && abs(params.progress - progress_hook_data.prev_progress) >= 0.01f) {
            fsm::PhaseData calibrating_data = { 0, 0, static_cast<uint8_t>(255 * params.progress), 1 };
            marlin_server::fsm_change(progress_hook_data.phase, calibrating_data);
            progress_hook_data.prev_progress = params.progress;
        }

        idle(true, true);
        return true;
    };

    for (size_t i = 0; i < spectrum.size(); ++i) {
        if (was_abort_requested(phase) || progress_hook_data.aborted) {
            return PhasesInputShaperCalibration::finish;
        }

        if (!args.calibrate_accelerometer) {
            data[2] = static_cast<uint8_t>(frequency);
        }
        data[3] = args.calibrate_accelerometer;

        marlin_server::fsm_change(phase, data);

        auto result = vibrate_measure_repeat(args, frequency, progress_hook);
        args.calibrate_accelerometer = false;
        if (!result.has_value()) {
            return PhasesInputShaperCalibration::measurement_failed;
        }

        result->gain[logicalAxis] = max(result->gain[logicalAxis] - 1.f, 0.f);
        spectrum.samples[i] = result->gain_square();
        frequency += frequency_range.increment;
    }
    spectrum.dump(logicalAxis == X_AXIS ? 'x' : 'y');
    if (spectrum.is_valid()) {
        return next_phase;
    } else {
        return PhasesInputShaperCalibration::measurement_failed;
    }
}

static PhasesInputShaperCalibration measuring_x_axis(Context &context) {
#if ENABLED(COREXY)
    // corexy doesn't need to attach_to_bed even with attachable accelerometer
    (void)HAS_ATTACHABLE_ACCELEROMETER();
    constexpr auto next_phase = PhasesInputShaperCalibration::measuring_y_axis;
#else
    #if HAS_ATTACHABLE_ACCELEROMETER()
    constexpr auto next_phase = PhasesInputShaperCalibration::attach_to_bed;
    #else
    // there is no bedslinger with permanent accelerometer at the moment, but we can dream...
    constexpr auto next_phase = PhasesInputShaperCalibration::measuring_y_axis;
    #endif
#endif
    return measuring_axis(
        PhasesInputShaperCalibration::measuring_x_axis,
        next_phase,
        X_AXIS,
        StepEventFlag::STEP_EVENT_FLAG_STEP_X,
        context.spectrum_x);
}

static PhasesInputShaperCalibration measuring_y_axis(Context &context) {
    return measuring_axis(
        PhasesInputShaperCalibration::measuring_y_axis,
        PhasesInputShaperCalibration::computing,
        Y_AXIS,
        StepEventFlag::STEP_EVENT_FLAG_STEP_Y,
        context.spectrum_y);
}

static PhasesInputShaperCalibration measurement_failed(Context &context) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::measurement_failed);
    switch (wait_for_response(PhasesInputShaperCalibration::measurement_failed)) {
    case Response::Retry:
        if (!context.spectrum_x.is_valid()) {
            return PhasesInputShaperCalibration::measuring_x_axis;
        }
        if (!context.spectrum_y.is_valid()) {
            return PhasesInputShaperCalibration::measuring_y_axis;
        }
        std::terminate();
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    default:
        std::terminate();
    }
    BUDDY_UNREACHABLE();
}

static PhasesInputShaperCalibration check_result(Context &context) {
    if (context.axis_config_x.frequency < input_shaper::low_freq_limit_hz || context.axis_config_x.frequency > input_shaper::high_freq_limit_hz
        || context.axis_config_y.frequency < input_shaper::low_freq_limit_hz || context.axis_config_y.frequency > input_shaper::high_freq_limit_hz) {
        return PhasesInputShaperCalibration::bad_results;
    } else {
        return PhasesInputShaperCalibration::results;
    }
}

static PhasesInputShaperCalibration computing(Context &context) {
    AxisEnum logicalAxis;
    bool aborted = false;
    const auto progress_hook = [&](input_shaper::Type type, float progress) {
        aborted |= was_abort_requested(PhasesInputShaperCalibration::computing);
        if (aborted) {
            return false;
        }

        fsm::PhaseData data {
            static_cast<uint8_t>(255 * progress),
            ftrstd::to_underlying(type),
            logicalAxis,
            0,
        };
        marlin_server::fsm_change(PhasesInputShaperCalibration::computing, data);
        idle(true, true);
        return true;
    };

    {
        logicalAxis = X_AXIS;
        context.axis_config_x = find_best_shaper(progress_hook, context.spectrum_x, input_shaper::axis_defaults[logicalAxis]);
        if (aborted) {
            return PhasesInputShaperCalibration::finish;
        }

        log_axis_config(context.axis_config_x, 'x');
    }

    {
        logicalAxis = Y_AXIS;
        context.axis_config_y = find_best_shaper(progress_hook, context.spectrum_y, input_shaper::axis_defaults[logicalAxis]);
        if (aborted) {
            return PhasesInputShaperCalibration::finish;
        }

        log_axis_config(context.axis_config_y, 'y');
    }

    return PhasesInputShaperCalibration::results;
}

static PhasesInputShaperCalibration results(Context &context) {
    const PhasesInputShaperCalibration result_phase = check_result(context); // results or bad_results - each has different texts & responses

    fsm::PhaseData data {
        static_cast<uint8_t>(context.axis_config_x.type),
        static_cast<uint8_t>(context.axis_config_x.frequency),
        static_cast<uint8_t>(context.axis_config_y.type),
        static_cast<uint8_t>(context.axis_config_y.frequency),
    };
    marlin_server::fsm_change(result_phase, data); // PhasesInputShaperCalibration::results or PhasesInputShaperCalibration::bad_results
    switch (wait_for_response(result_phase)) {
    case Response::Yes:
        config_store().input_shaper_axis_x_config.set(context.axis_config_x);
        input_shaper::set_axis_config(X_AXIS, context.axis_config_x);
        config_store().input_shaper_axis_y_config.set(context.axis_config_y);
        input_shaper::set_axis_config(Y_AXIS, context.axis_config_y);
        set_test_result(TestResult_Passed);
        return PhasesInputShaperCalibration::finish;
    case Response::No:
        set_test_result(TestResult_Skipped);
        return PhasesInputShaperCalibration::finish;
    case Response::Ok:
        set_test_result(TestResult_Skipped);
        return PhasesInputShaperCalibration::finish;
    default:
        break;
    }
    BUDDY_UNREACHABLE();
}

static PhasesInputShaperCalibration finish(Context &context) {
#if HAS_PHASE_STEPPING()
    context.phstep_restorer.release();
#else
    UNUSED(context);
#endif
    return PhasesInputShaperCalibration::finish;
}

static PhasesInputShaperCalibration get_next_phase(Context &context, const PhasesInputShaperCalibration phase) {
    switch (phase) {
    case PhasesInputShaperCalibration::info:
        return info();
    case PhasesInputShaperCalibration::parking:
        return parking(context);
#if HAS_ATTACHABLE_ACCELEROMETER()
    case PhasesInputShaperCalibration::connect_to_board:
        return connect_to_board(context);
    case PhasesInputShaperCalibration::wait_for_extruder_temperature:
        return wait_for_extruder_temperature(context);
    case PhasesInputShaperCalibration::attach_to_extruder:
        return attach_to_extruder(context);
    case PhasesInputShaperCalibration::attach_to_bed:
        return attach_to_bed(context);
#endif
    case PhasesInputShaperCalibration::measuring_x_axis:
        return measuring_x_axis(context);
    case PhasesInputShaperCalibration::measuring_y_axis:
        return measuring_y_axis(context);
    case PhasesInputShaperCalibration::measurement_failed:
        return measurement_failed(context);
    case PhasesInputShaperCalibration::computing:
        return computing(context);
    case PhasesInputShaperCalibration::bad_results:
    case PhasesInputShaperCalibration::results:
        return results(context);
    case PhasesInputShaperCalibration::finish:
        return finish(context);
    }
    std::terminate();
}

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1959: Input Shaper Calibration Wizard
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1959
 *
 */

void M1959() {
    Context context;
    PhasesInputShaperCalibration phase = PhasesInputShaperCalibration::info;
    marlin_server::FSM_Holder holder { phase };
    do {
        phase = get_next_phase(context, phase);
    } while (phase != PhasesInputShaperCalibration::finish);
}

/** @}*/

} // namespace PrusaGcodeSuite
