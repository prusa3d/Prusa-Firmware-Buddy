#include "M1959.hpp"
#include "module/tool_change.h"

#include <common/marlin_server.hpp>
#include <common/str_utils.hpp>
#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/gcode/calibrate/M958.hpp>
#include <option/development_items.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_local_accelerometer.h>
#include <option/has_remote_accelerometer.h>

static_assert(HAS_INPUT_SHAPER_CALIBRATION());

static constexpr bool klipper_mode = true;
static constexpr float acceleration_requested = 2.5f;
static constexpr uint32_t cycles = 50;

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
    float accelerometer_sample_period;
    FrequencyRangeSpectrum spectrum_x;
    FrequencyRangeSpectrum spectrum_y;
    input_shaper::AxisConfig axis_config_x;
    input_shaper::AxisConfig axis_config_y;

    bool is_accelerometer_ok() const {
        return accelerometer && accelerometer->get_error() == PrusaAccelerometer::Error::none;
    }

    void ensure_accelerometer_ok() {
        // Maybe redscreen would be better. Maybe retry would be better.
        // Let's see how this behaves in testing department and fix accordingly.
        if (!is_accelerometer_ok()) {
            bsod("ensure_accelerometer_ok");
        }
    }
};

static PhasesInputShaperCalibration dispatch_accelerometer(Context &context) {
    // Note: We need to explicitly destroy the accelerometer.
    //       When assigning to std::unique_ptr it calls the destructor of the original
    //       object _after_ the construction of the new object to provide
    //       strong exception safety. This would setup the pin in the constructor
    //       and then immediately revert it in the destructor which is not what we want.
    context.accelerometer = nullptr;
    context.accelerometer = std::make_unique<PrusaAccelerometer>();
    if (context.is_accelerometer_ok()) {
#if HAS_LOCAL_ACCELEROMETER()
        return PhasesInputShaperCalibration::attach_to_extruder;
#else
        return PhasesInputShaperCalibration::calibrating_accelerometer;
#endif
    } else {
#if HAS_LOCAL_ACCELEROMETER()
        return PhasesInputShaperCalibration::connect_to_board;
#else
        bsod(__FUNCTION__);
#endif
    }
}

static PhasesInputShaperCalibration info(Context &) {
    switch (wait_for_response(PhasesInputShaperCalibration::info)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    case Response::Continue:
        return PhasesInputShaperCalibration::parking;
    default:
        std::terminate();
    }
}

static PhasesInputShaperCalibration parking(Context &context) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::parking);

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

    // Easier access to cables + more consistent measurement
    const xyz_pos_t pos = { X_BED_SIZE / 2, Y_BED_SIZE / 2, Z_SIZE / 2 };
    plan_park_move_to_xyz(pos, HOMING_FEEDRATE_XY, HOMING_FEEDRATE_Z);

    // Carry on the changes
    planner.synchronize();
    return dispatch_accelerometer(context);
}

static PhasesInputShaperCalibration connect_to_board(Context &context) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::connect_to_board);
    switch (wait_for_response(PhasesInputShaperCalibration::connect_to_board)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    case Response::Retry:
        return dispatch_accelerometer(context);
    default:
        break;
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
    context.ensure_accelerometer_ok();
    AccelerometerProgressHookFsm progress_hook;
    context.accelerometer_sample_period = get_accelerometer_sample_period(progress_hook, *context.accelerometer);
    if (isnan(context.accelerometer_sample_period)) {
        bsod("Accelerometer calibration failed.");
    }
    return progress_hook.aborted() ? PhasesInputShaperCalibration::finish : PhasesInputShaperCalibration::measuring_x_axis;
}

// helper; return true if aborted
static bool measuring_axis(
    Context &context,
    const PhasesInputShaperCalibration phase,
    const AxisEnum logicalAxis,
    const StepEventFlag_t axis_flag,
    FrequencyRangeSpectrum &spectrum) {

    context.ensure_accelerometer_ok();
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
            return true;
        }
        data[2] = static_cast<uint8_t>(frequency_requested);
        marlin_server::fsm_change(phase, data);

        FrequencyGain3D frequencyGain3D = vibrate_measure(*context.accelerometer, context.accelerometer_sample_period, axis_flag, klipper_mode, frequency_requested, acceleration_requested, step_len, cycles);
        frequencyGain3D.gain[logicalAxis] = max(frequencyGain3D.gain[logicalAxis] - 1.f, 0.f);
        spectrum.samples[i] = frequencyGain3D.get_square();
        frequency_requested += frequency_range.increment;
    }
    return false;
}

static PhasesInputShaperCalibration measuring_x_axis(Context &context) {
    FrequencyRangeSpectrum &spectrum = context.spectrum_x;
    if (measuring_axis(context, PhasesInputShaperCalibration::measuring_x_axis, X_AXIS, StepEventFlag::STEP_EVENT_FLAG_STEP_X, spectrum)) {
        return PhasesInputShaperCalibration::finish;
    }
    spectrum.dump('x');
    if (!spectrum.is_valid()) {
        return PhasesInputShaperCalibration::measurement_failed;
    }
#if HAS_LOCAL_ACCELEROMETER()
    return PhasesInputShaperCalibration::attach_to_bed;
#else
    return PhasesInputShaperCalibration::measuring_y_axis;
#endif
}

static PhasesInputShaperCalibration attach_to_bed(Context &) {
    marlin_server::fsm_change(PhasesInputShaperCalibration::attach_to_bed);
    switch (wait_for_response(PhasesInputShaperCalibration::attach_to_bed)) {
    case Response::Abort:
        return PhasesInputShaperCalibration::finish;
    case Response::Continue:
        return PhasesInputShaperCalibration::measuring_y_axis;
        ;
    default:
        break;
    }
    bsod(__FUNCTION__);
}

static PhasesInputShaperCalibration measuring_y_axis(Context &context) {
    FrequencyRangeSpectrum &spectrum = context.spectrum_y;
    if (measuring_axis(context, PhasesInputShaperCalibration::measuring_y_axis, Y_AXIS, StepEventFlag::STEP_EVENT_FLAG_STEP_Y, spectrum)) {
        return PhasesInputShaperCalibration::finish;
    }
    spectrum.dump('y');
    if (!spectrum.is_valid()) {
        return PhasesInputShaperCalibration::measurement_failed;
    }
    return PhasesInputShaperCalibration::computing;
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

static PhasesInputShaperCalibration computing(Context &context) {
    FindBestShaperProgressHookFsm progress_hook;
    {
        const AxisEnum logicalAxis = X_AXIS;
        progress_hook.set_axis(logicalAxis);
        context.axis_config_x = find_best_shaper(progress_hook, context.spectrum_x, input_shaper::axis_defaults[logicalAxis]);
    }
    {
        const AxisEnum logicalAxis = Y_AXIS;
        progress_hook.set_axis(logicalAxis);
        context.axis_config_y = find_best_shaper(progress_hook, context.spectrum_y, input_shaper::axis_defaults[logicalAxis]);
    }
    return progress_hook.aborted() ? PhasesInputShaperCalibration::finish : PhasesInputShaperCalibration::results;
}

static PhasesInputShaperCalibration results(Context &context) {
    fsm::PhaseData data {
        static_cast<uint8_t>(context.axis_config_x.type),
        static_cast<uint8_t>(context.axis_config_x.frequency),
        static_cast<uint8_t>(context.axis_config_y.type),
        static_cast<uint8_t>(context.axis_config_y.frequency),
    };
    marlin_server::fsm_change(PhasesInputShaperCalibration::results, data);
    switch (wait_for_response(PhasesInputShaperCalibration::results)) {
    case Response::Yes:
        config_store().input_shaper_axis_x_config.set(context.axis_config_x);
        input_shaper::set_axis_config(X_AXIS, context.axis_config_x);
        config_store().input_shaper_axis_y_config.set(context.axis_config_y);
        input_shaper::set_axis_config(Y_AXIS, context.axis_config_y);
        return PhasesInputShaperCalibration::finish;
    case Response::No:
        return PhasesInputShaperCalibration::finish;
    default:
        break;
    }
    bsod(__FUNCTION__);
}

static PhasesInputShaperCalibration get_next_phase(Context &context, const PhasesInputShaperCalibration phase) {
    switch (phase) {
    case PhasesInputShaperCalibration::info:
        return info(context);
    case PhasesInputShaperCalibration::parking:
        return parking(context);
    case PhasesInputShaperCalibration::connect_to_board:
        return connect_to_board(context);
    case PhasesInputShaperCalibration::calibrating_accelerometer:
        return calibrating_accelerometer(context);
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
    case PhasesInputShaperCalibration::results:
        return results(context);
    case PhasesInputShaperCalibration::finish:
        return PhasesInputShaperCalibration::finish;
    }
    std::terminate();
}

namespace PrusaGcodeSuite {

void M1959() {
    Context context;
    PhasesInputShaperCalibration phase = PhasesInputShaperCalibration::info;
    marlin_server::FSM_Holder holder { phase };
    do {
        phase = get_next_phase(context, phase);
    } while (phase != PhasesInputShaperCalibration::finish);
}

} // namespace PrusaGcodeSuite
