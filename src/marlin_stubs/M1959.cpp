#include "M1959.hpp"

#include <common/marlin_server.hpp>
#include <common/str_utils.hpp>
#include <Marlin/src/gcode/calibrate/M958.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/tool_change.h>
#include <option/development_items.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_local_accelerometer.h>
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

static Response wait_for_response(const PhasesInputShaperCalibration phase) {
    for (;;) {
        const Response response = marlin_server::get_response_from_phase(phase);
        if (response == Response::_none) {
            idle(true); // prevent watchdog reset
        } else {
            return response;
        }
    }
}

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
        std::array<char, 1 + strlen_constexpr("YYYYmmddHHMMSS")> strftime_buffer;
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
    // Note: ptr is needed because accelerometer does init and doesn't have reinit
    std::unique_ptr<PrusaAccelerometer> accelerometer;
    float accelerometer_sample_period { NAN };
    FrequencyRangeSpectrum spectrum_x;
    FrequencyRangeSpectrum spectrum_y;
    input_shaper::AxisConfig axis_config_x;
    input_shaper::AxisConfig axis_config_y;
#if HAS_PHASE_STEPPING()
    phase_stepping::StateRestorer phstep_restorer;
#endif

    bool is_accelerometer_ok() const {
        assert(accelerometer);
        return accelerometer->get_error() == PrusaAccelerometer::Error::none;
    }

    bool setup_accelerometer() {
        accelerometer.reset();
        accelerometer = std::make_unique<PrusaAccelerometer>();
        return is_accelerometer_ok();
    }
};

static PhasesInputShaperCalibration info_proceed(Context &context) {
    if (context.setup_accelerometer()) {
        return PhasesInputShaperCalibration::parking;
    }
    return PhasesInputShaperCalibration::connect_to_board;
}

static PhasesInputShaperCalibration info_factory(Context &context) {
    switch (wait_for_response(PhasesInputShaperCalibration::info_factory)) {
    case Response::Yes:
        return info_proceed(context);
    case Response::No:
        set_test_result(TestResult_Passed);
        return PhasesInputShaperCalibration::finish;
    default:
        std::terminate();
    }
}

static PhasesInputShaperCalibration info_calibrated(Context &context) {
    switch (wait_for_response(PhasesInputShaperCalibration::info_calibrated)) {
    case Response::Continue:
        return info_proceed(context);
    case Response::Abort:
        // do not set_test_result()
        return PhasesInputShaperCalibration::finish;
    default:
        std::terminate();
    }
}

// Note: This is only relevant for printers which HAS_LOCAL_ACCELEROMETER()
//       which coincidentally only have one hotend.
static constexpr uint8_t hotend = 0;
static constexpr float safe_temperature = 50;

static PhasesInputShaperCalibration parking(Context &context) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::parking);

#if HAS_LOCAL_ACCELEROMETER()
    // Start cooling the hotend even before parking to save some time
    Temperature::disable_hotend();
    if (Temperature::degHotend(hotend) > safe_temperature) {
        Temperature::set_fan_speed(hotend, 255);
    }
#endif

    // Home if not homed
    if (!all_axes_known()) {
        GcodeSuite::G28_no_parser(false, true, 0, false, true, true, true);
    }

#if HAS_REMOTE_ACCELEROMETER()
    // Without tool being picked there is no accelerometer data
    if (prusa_toolchanger.has_tool() == false) {
        tool_change(/*tool_index=*/0, tool_return_t::no_return, tool_change_lift_t::no_lift, /*z_down=*/false);
    }
#endif

    // Ensure consistent measurement
    const xyz_pos_t pos = { X_BED_SIZE / 2, Y_BED_SIZE / 2, Z_SIZE / 2 };
    plan_park_move_to_xyz(pos, HOMING_FEEDRATE_XY, HOMING_FEEDRATE_Z);

#if HAS_PHASE_STEPPING()
    // Ensure phase stepping is disabled throughout the calibration as we manipulate steps directly
    context.phstep_restorer.set_state(false);
#else
    std::ignore = context;
#endif

    // Carry on the changes
    planner.synchronize();
#if HAS_LOCAL_ACCELEROMETER()
    return PhasesInputShaperCalibration::wait_for_extruder_temperature;
#else
    return PhasesInputShaperCalibration::calibrating_accelerometer;
#endif
}

static PhasesInputShaperCalibration connect_to_board(Context &) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::connect_to_board);
    switch (wait_for_response(PhasesInputShaperCalibration::connect_to_board)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    default:
        break;
    }
    bsod(__FUNCTION__);
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
            bsod(__FUNCTION__);
        }
    }
    bsod(__FUNCTION__);
}

static PhasesInputShaperCalibration attach_to_extruder(Context &) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::attach_to_extruder);
    switch (wait_for_response(PhasesInputShaperCalibration::attach_to_extruder)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    case Response::Continue:
        return PhasesInputShaperCalibration::calibrating_accelerometer;
    default:
        break;
    }
    bsod(__FUNCTION__);
}

class AccelerometerProgressHookFsm final : public AccelerometerProgressHook {
private:
    bool m_aborted = false;

public:
    AccelerometerProgressHookFsm() {
        std::ignore = operator()(0.0f);
    }

    ProgressResult operator()(float progress_ratio) final {
        if (m_aborted || was_abort_requested(PhasesInputShaperCalibration::calibrating_accelerometer)) {
            m_aborted = true;
            return ProgressResult::abort;
        } else {
            fsm::PhaseData data = { static_cast<uint8_t>(255 * progress_ratio), 0, 0, 0 };
            marlin_server::fsm_change(PhasesInputShaperCalibration::calibrating_accelerometer, data);
            idle(true, true);
            return ProgressResult::progress;
        }
    }

    bool aborted() const { return m_aborted; }
};

static PhasesInputShaperCalibration calibrating_accelerometer(Context &context) {
    if (!context.setup_accelerometer()) {
        return PhasesInputShaperCalibration::measurement_failed;
    }

    AccelerometerProgressHookFsm progress_hook;
    context.accelerometer_sample_period = get_accelerometer_sample_period(progress_hook, *context.accelerometer);
    if (isnan(context.accelerometer_sample_period)) {
        return PhasesInputShaperCalibration::measurement_failed;
    }
    return progress_hook.aborted() ? PhasesInputShaperCalibration::finish : PhasesInputShaperCalibration::measuring_x_axis;
}

// helper
static PhasesInputShaperCalibration measuring_axis(
    Context &context,
    const PhasesInputShaperCalibration phase,
    const PhasesInputShaperCalibration next_phase,
    const AxisEnum logicalAxis,
    const StepEventFlag_t axis_flag,
    FrequencyRangeSpectrum &spectrum) {
    assert(!isnan(context.accelerometer_sample_period));
    if (!context.setup_accelerometer()) {
        return PhasesInputShaperCalibration::measurement_failed;
    }

    fsm::PhaseData data {
        static_cast<uint8_t>(frequency_range.start),
        static_cast<uint8_t>(frequency_range.end),
        static_cast<uint8_t>(frequency_range.start), // current frequency
        0,
    };
    marlin_server::fsm_change(phase, data);

    // phstep needs to be off _before_ getting the current ustep resolution
    phase_stepping::EnsureDisabled phaseSteppingDisabler;
    MicrostepRestorer microstepRestorer;
    enable_all_steppers(); // enable all axes to have the same state as printing

    stepper_microsteps(logicalAxis, 128);
    const float step_len = get_step_len(axis_flag, microstepRestorer.saved_mres());
    if (isnan(step_len)) {
        bsod("isnan(step_len)");
    }

    float frequency_requested = frequency_range.start;
    for (size_t i = 0; i < spectrum.size(); ++i) {
        if (was_abort_requested(phase)) {
            return PhasesInputShaperCalibration::finish;
        }
        data[2] = static_cast<uint8_t>(frequency_requested);
        marlin_server::fsm_change(phase, data);

        FrequencyGain3dError frequencyGain3dError = vibrate_measure(*context.accelerometer, context.accelerometer_sample_period, axis_flag, klipper_mode, frequency_requested, acceleration_requested, step_len, cycles);
        if (frequencyGain3dError.error) {
            return PhasesInputShaperCalibration::measurement_failed;
        }

        frequencyGain3dError.frequencyGain3d.gain[logicalAxis] = max(frequencyGain3dError.frequencyGain3d.gain[logicalAxis] - 1.f, 0.f);
        spectrum.samples[i] = frequencyGain3dError.frequencyGain3d.get_square();
        frequency_requested += frequency_range.increment;
    }
    spectrum.dump(logicalAxis == X_AXIS ? 'x' : 'y');
    if (spectrum.is_valid()) {
        return next_phase;
    } else {
        return PhasesInputShaperCalibration::measurement_failed;
    }
}

static PhasesInputShaperCalibration measuring_x_axis(Context &context) {
#if HAS_LOCAL_ACCELEROMETER()
    constexpr auto next_phase = PhasesInputShaperCalibration::attach_to_bed;
#else
    constexpr auto next_phase = PhasesInputShaperCalibration::measuring_y_axis;
#endif
    return measuring_axis(
        context,
        PhasesInputShaperCalibration::measuring_x_axis,
        next_phase,
        X_AXIS,
        StepEventFlag::STEP_EVENT_FLAG_STEP_X,
        context.spectrum_x);
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
    bsod(__FUNCTION__);
}

static PhasesInputShaperCalibration measuring_y_axis(Context &context) {
    return measuring_axis(
        context,
        PhasesInputShaperCalibration::measuring_y_axis,
        PhasesInputShaperCalibration::computing,
        Y_AXIS,
        StepEventFlag::STEP_EVENT_FLAG_STEP_Y,
        context.spectrum_y);
}

class FindBestShaperProgressHookFsm final : public FindBestShaperProgressHook {
private:
    bool m_aborted = false;
    AxisEnum m_axis = AxisEnum::NO_AXIS_ENUM;

public:
    FindBestShaperProgressHookFsm() {
        std::ignore = operator()(input_shaper::Type::null, 0.0f);
    }

    ProgressResult operator()(input_shaper::Type type, float progress_ratio) final {
        if (m_aborted || was_abort_requested(PhasesInputShaperCalibration::computing)) {
            m_aborted = true;
            return ProgressResult::abort;
        } else {
            fsm::PhaseData data {
                static_cast<uint8_t>(255 * progress_ratio),
                ftrstd::to_underlying(type),
                m_axis,
                0,
            };
            marlin_server::fsm_change(PhasesInputShaperCalibration::computing, data);
            idle(true, true);
            return ProgressResult::progress;
        }
    }

    void set_axis(AxisEnum axis) { m_axis = axis; } // TODO maybe this belongs to the progress hook...

    bool aborted() const { return m_aborted; }
};

static PhasesInputShaperCalibration measurement_failed(Context &context) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::measurement_failed);
    switch (wait_for_response(PhasesInputShaperCalibration::measurement_failed)) {
    case Response::Retry:
        if (isnan(context.accelerometer_sample_period)) {
            return PhasesInputShaperCalibration::calibrating_accelerometer;
        }
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
    bsod(__FUNCTION__);
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
    FindBestShaperProgressHookFsm progress_hook;
    {
        const AxisEnum logicalAxis = X_AXIS;
        progress_hook.set_axis(logicalAxis);
        context.axis_config_x = find_best_shaper(progress_hook, context.spectrum_x, input_shaper::axis_defaults[logicalAxis]);
        log_axis_config(context.axis_config_x, 'x');
    }
    {
        const AxisEnum logicalAxis = Y_AXIS;
        progress_hook.set_axis(logicalAxis);
        context.axis_config_y = find_best_shaper(progress_hook, context.spectrum_y, input_shaper::axis_defaults[logicalAxis]);
        log_axis_config(context.axis_config_y, 'y');
    }

    return progress_hook.aborted() ? PhasesInputShaperCalibration::finish : PhasesInputShaperCalibration::results;
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
    bsod(__FUNCTION__);
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
    case PhasesInputShaperCalibration::info_factory:
        return info_factory(context);
    case PhasesInputShaperCalibration::info_calibrated:
        return info_calibrated(context);
    case PhasesInputShaperCalibration::parking:
        return parking(context);
    case PhasesInputShaperCalibration::connect_to_board:
        return connect_to_board(context);
    case PhasesInputShaperCalibration::calibrating_accelerometer:
        return calibrating_accelerometer(context);
    case PhasesInputShaperCalibration::wait_for_extruder_temperature:
        return wait_for_extruder_temperature(context);
    case PhasesInputShaperCalibration::attach_to_extruder:
        return attach_to_extruder(context);
    case PhasesInputShaperCalibration::measuring_x_axis:
        return measuring_x_axis(context);
    case PhasesInputShaperCalibration::attach_to_bed:
        return attach_to_bed(context);
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

void M1959() {
    Context context;
    const bool factory = config_store().selftest_result_input_shaper_calibration.get() == TestResult_Unknown;
    PhasesInputShaperCalibration phase = factory ? PhasesInputShaperCalibration::info_factory : PhasesInputShaperCalibration::info_calibrated;
    marlin_server::FSM_Holder holder { phase };
    do {
        phase = get_next_phase(context, phase);
    } while (phase != PhasesInputShaperCalibration::finish);
}

} // namespace PrusaGcodeSuite
