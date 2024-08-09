/**
 * @file
 * @author: Marek Bel
 *
 * This code is based on Klipper project.
 * Author: Dmitry Butyugin <dmbutyugin@google.com>
 * Source: https://github.com/Klipper3d/klipper/blob/master/klippy/extras/shaper_calibrate.py
 */
#include "M958.hpp"

#include "../../inc/MarlinConfig.h"
#include "../gcode.h"
#include "../../module/planner.h"
#include "../../Marlin.h"
#include "../../module/stepper/trinamic.h"
#include "../../module/prusa/accelerometer.h"
#include "../../feature/input_shaper/input_shaper.hpp"
#include "metric.h"
#include <cmath>
#include <complex>
#include <numbers>
#include <limits>
#include <bit>
#include <utility_extensions.hpp>

#include <config_store/store_instance.hpp>

#include <option/has_local_accelerometer.h>
#include <option/has_dwarf.h>
#include <option/has_remote_accelerometer.h>

static_assert(HAS_LOCAL_ACCELEROMETER() || HAS_REMOTE_ACCELEROMETER());

// #define M958_OUTPUT_SAMPLES
// #define M958_VERBOSE
#ifdef M958_OUTPUT_SAMPLES
    #include "../../../../../tinyusb/src/class/cdc/cdc_device.h"
#endif

METRIC_DEF(metric_excite_freq, "excite_freq", METRIC_VALUE_FLOAT, 100, METRIC_HANDLER_DISABLE_ALL);
METRIC_DEF(metric_freq_gain, "freq_gain", METRIC_VALUE_CUSTOM, 100, METRIC_HANDLER_ENABLE_ALL);
METRIC_DEF(accel, "tk_accel", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);

namespace {
class HarmonicGenerator {

public:
    /**
     * @brief Displacement amplitude
     *
     * double integral of acceleration over time results in position amplitude
     *
     * @param acceleration in m/s-2
     * @param frequency in Hz
     * @return amplitude in meters
     */
    static float amplitudeNotRounded(float frequency, float acceleration) {
        return acceleration / (4 * std::numbers::pi_v<float> * std::numbers::pi_v<float> * frequency * frequency);
    }
    static int amplitudeRoundToSteps(float amplitude_not_rounded, float step_len) {
        return ceil(amplitude_not_rounded / step_len);
    }
    HarmonicGenerator(float frequency, float amplitude, float step_len)
        : m_amplitude_steps(amplitudeRoundToSteps(amplitude, step_len))
        , m_step(step_len)
        , m_freq2pi_inv(1.f / (frequency * 2 * std::numbers::pi_v<float>))
        , m_last_time(1.f / (frequency * 4.f))
        , m_last_step(m_amplitude_steps - 1)
        , m_dir_forward(false) {}

    float nextDelayDir() {
        float new_time = asinf(static_cast<float>(m_last_step) / m_amplitude_steps) * m_freq2pi_inv;

        if (m_dir_forward) {
            if (m_last_step < m_amplitude_steps) {
                ++m_last_step;
            } else {
                --m_last_step;
                m_dir_forward = false;
            }
        } else {
            if (m_last_step > -m_amplitude_steps) {
                --m_last_step;
            } else {
                ++m_last_step;
                m_dir_forward = true;
            }
        }

        float next_delay = new_time - m_last_time;
        m_last_time = new_time;
        return next_delay;
    }

    int getStepsPerPeriod() {
        return (m_amplitude_steps * 4);
    }

    float getFrequency() {
        float period = .0f;
        for (int i = 0; i < getStepsPerPeriod(); ++i) {
            period += abs(nextDelayDir());
        }
        return 1.f / period;
    }

    float getAcceleration(float frequency) {
        return m_amplitude_steps * m_step * 4.f * std::numbers::pi_v<float> * std::numbers::pi_v<float> * frequency * frequency;
    }

private:
    const int m_amplitude_steps; ///< amplitude rounded to steps
    const float m_step;
    const float m_freq2pi_inv;
    float m_last_time;
    int m_last_step;
    bool m_dir_forward;
};

class StepDir {
public:
    static constexpr float m_ticks_per_second = STEPPER_TIMER_RATE;
    struct RetVal {
        int step_us;
        bool dir;
    };
    StepDir(HarmonicGenerator &generator)
        : m_generator(generator)
        , m_step_us_fraction(.0f) {}
    RetVal get() {
        RetVal retval(0);
        const float next_delay_dir = m_generator.nextDelayDir();
        retval.dir = signbit(next_delay_dir);

        const float next_delay = abs(next_delay_dir);
        const float next_delay_us = next_delay * m_ticks_per_second + m_step_us_fraction;
        retval.step_us = next_delay_us;
        m_step_us_fraction = next_delay_us - retval.step_us;

        return retval;
    }

private:
    HarmonicGenerator &m_generator;
    float m_step_us_fraction;
};

template <size_t max_samples>
class FixedLengthSpectrum final : public Spectrum {
public:
    FixedLengthSpectrum(float start_frequency, float frequency_step)
        : m_gain()
        , m_start_frequency(start_frequency)
        , m_frequency_step(frequency_step)
        , m_size(0) {}

    constexpr size_t max_size() const { return max_samples; }

    size_t size() const final { return m_size; }

    void put(float gain) {
        if (m_size >= max_samples) {
            return;
        }
        m_gain[m_size] = gain;
        ++m_size;
    }
    FrequencyGain get(size_t index) const final {
        FrequencyGain retval = { 0.f, 0.f };
        if (index < m_size) {
            retval.frequency = m_start_frequency + index * m_frequency_step;
            retval.gain = m_gain[index];
        }
        return retval;
    }
    float max() const final {
        float maximum = std::numeric_limits<float>::min();
        for (size_t i = 0; i < m_size; ++i) {
            if (m_gain[i] > maximum) {
                maximum = m_gain[i];
            }
        }
        return maximum;
    }

private:
    float m_gain[max_samples];
    float m_start_frequency;
    float m_frequency_step;
    size_t m_size;
};

} // anonymous namespace

static bool is_ok(PrusaAccelerometer::Error error);

static bool is_full() {
    CRITICAL_SECTION_START;
    bool retval = PreciseStepping::is_step_event_queue_full();
    CRITICAL_SECTION_END;
    return retval;
}

static void enqueue_step(int step_us, bool dir, StepEventFlag_t axis_flags) {
    assert(step_us <= STEP_TIMER_MAX_TICKS_LIMIT);
    uint16_t next_queue_head = 0;

    CRITICAL_SECTION_START;
    step_event_u16_t *step_event = PreciseStepping::get_next_free_step_event(next_queue_head);
    step_event->time_ticks = step_us;
    step_event->flags = axis_flags;
    if (dir) {
        step_event->flags ^= STEP_EVENT_FLAG_DIR_MASK;
    }
    PreciseStepping::step_event_queue.head = next_queue_head;
    CRITICAL_SECTION_END;
}

struct Accumulator {
    std::complex<double> val[3];
};

/**
 * @brief Get recommended damping ratio for zv input shaper
 *
 * This is probably not right. This computation is based on assumption filter should damp
 * system resonant gain to 1. But from the theory of input shaper comes requirement that
 * resonant frequency of the system should be either damped to zero or the oscillation
 * excited should be immediately cancelled out.
 *
 * zv shaper gain computed as
 * https://www.wolframalpha.com/input?i=g%3D50%2C+f%3D50%2C+d%3D0.1%2C+%28sin%28x*2pi*g%29%2Be%5E%28-d*pi%2Fsqrt%281-d%5E2%29%29*sin%28%28x%2B1%2F%282*f*sqrt%281-d%5E2%29%29%29*2pi*g%29%29%2F%281%2Be%5E%28-d*pi%2Fsqrt%281-d%5E2%29%29%29
 * where g .. frequency probed[Hz], f .. shaper maximum atenuation frequency[Hz], d .. SHAPER_DAMPING_RATIO[-]
 * It computes output signal of the filter for input signal is sine wave with amplitude 1.
 * So if output amplitude (either visible from graph or computed from Reduced trigonometric form) is 0.15,
 * it means filter gain at that frequency is 0.15.
 *
 * Pre-computed values were tabulated
 *
 * | shaper gain       | damping ratio  |
 * | ----------------- | -------------- |
 * | 0.0157076902629   | 0.01           |
 * | 0.0785009         | 0.05           |
 * | 0.15676713        | 0.1            |
 * | 0.234556          | 0.15           |
 * | 0.311608          | 0.2            |
 * | 0.738780338281116 | 0.5            |
 *
 * and approximated by quadratic function.
 *
 * @param resonant_gain gain of resonator to be damped to 1
 * @return recommended zv shaper damping_ratio
 *
 */
static float get_zv_shaper_damping_ratio(float resonant_gain) {
    float shaper_gain = 1.f / resonant_gain;
    return 0.080145136132399f * sq(shaper_gain) + 0.616396503538947f * shaper_gain + 0.000807776046666f;
}

static constexpr float expected_accelerometer_sample_period = 1.f / 1344.f;

/**
 * @param accelerometer
 * @return accelerometer sample period in seconds
 * @retval NAN error
 */
float get_accelerometer_sample_period(const SamplePeriodProgressHook &progress_hook, PrusaAccelerometer &accelerometer) {
    for (int i = 0; i < 96; ++i) {
        // Note: this is fast enough, it does not need to call progress_hook
        idle(true, true);
        accelerometer.clear();
    }
    constexpr int request_samples_num = 20'000;
    constexpr uint32_t max_duration_ms = 2.f * 1000.f * expected_accelerometer_sample_period * request_samples_num;
    const uint32_t start_time = millis();
    uint32_t duration_ms = 0;

    if (!progress_hook(0)) {
        return NAN;
    }

    for (int i = 0; i < request_samples_num;) {
        PrusaAccelerometer::Acceleration measured_acceleration;
        const int samples = accelerometer.get_sample(measured_acceleration);
        if (samples) {
            ++i;

        } else if (!progress_hook(static_cast<float>(i) / request_samples_num)) {
            return NAN;
        }

        const uint32_t now = millis();
        duration_ms = now - start_time;

        if (duration_ms > max_duration_ms) {
            SERIAL_ERROR_MSG("sample period: getting accelerometer samples timed out");
            (void)is_ok(accelerometer.get_error());
            return NAN;
        }
    }

    if (!is_ok(accelerometer.get_error())) {
        return NAN;
    }

    return duration_ms / 1000.f / static_cast<float>(request_samples_num);
}

static float maybe_calibrate_and_get_accelerometer_sample_period(PrusaAccelerometer &accelerometer, bool calibrate_accelerometer, const SamplePeriodProgressHook &progress_hook) {
    // TODO: Perhaps we should always calibrate accelerometer and not use this global variable...
    //       Then again, maybe we should not have M958 in the first place...
    static float sample_period = expected_accelerometer_sample_period;
    if (calibrate_accelerometer || isnan(sample_period)) {
        sample_period = get_accelerometer_sample_period(progress_hook, accelerometer);
        SERIAL_ECHOLNPAIR_F("Sample freq: ", 1.f / sample_period);
    }
    return sample_period;
}

static void serial_echo_header(bool klipper_mode) {
    if (klipper_mode) {
        SERIAL_ECHOLNPGM("freq,psd_x,psd_y,psd_z,psd_xyz,mzv");
    } else {
        SERIAL_ECHOLNPGM("excitation_frequency[Hz] measurement_frequency[Hz] excitation[m/s^2] X[m/s^2] Y[m/s^2] Z[m/s^2] X_gain Y_gain Z_gain");
    }
}

/**
 * @brief Get logical axis from motor axis_flag
 *
 * @param axis_flag motors and initial directions flags see StepEventFlag
 * @retval !NO_AXIS_ENUM for single logical axis if vibrations are generated aligned for that particular single logical axis only
 * @retval NO_AXIS_ENUM for all logical axis if the move is not parallel to single logical axis - e.g. diagonal movement or no movement
 */
AxisEnum get_logical_axis(const uint16_t axis_flag) {
    const bool x_flag = axis_flag & STEP_EVENT_FLAG_STEP_X;
    const bool y_flag = axis_flag & STEP_EVENT_FLAG_STEP_Y;
    const bool z_flag = axis_flag & STEP_EVENT_FLAG_STEP_Z;
#if IS_CARTESIAN
    if (z_flag) {
        return (!x_flag && !y_flag ? Z_AXIS : NO_AXIS_ENUM);
    }

    #if IS_CORE
        #if CORE_IS_XY
    if (x_flag == y_flag) {
        const bool x_dir = axis_flag & STEP_EVENT_FLAG_X_DIR;
        const bool y_dir = axis_flag & STEP_EVENT_FLAG_Y_DIR;
        return (x_dir == y_dir ? X_AXIS : Y_AXIS);
    }
        #else
            #error "Not implemented."
        #endif
    #else
    if (x_flag != y_flag) {
        return (x_flag ? X_AXIS : Y_AXIS);
    }
    #endif
#else
    #error "Not implemented."
#endif
    return NO_AXIS_ENUM;
}

bool VibrateMeasureParams::setup(const MicrostepRestorer &microstep_restorer) {
    step_len = get_step_len(axis_flag, microstep_restorer.saved_mres());
    if (isnan(step_len)) {
        return false;
    }
    return true;
}

static FrequencyGain3dError vibrate_measure(const VibrateMeasureParams &args, float requested_frequency, const SamplePeriodProgressHook &progress_hook) {
    if (args.klipper_mode && args.measured_harmonic != 1) {
        SERIAL_ERROR_MSG("vibrate measure: klipper mode does not support measuring higher harmonics");
        return FrequencyGain3dError { .error = true };
    }

    // As we push steps directly, phase stepping needs to be off
    phase_stepping::assert_disabled();

    // Check that exactly one of excitation_acceleration, excitation_amplitude is specified
    assert(isnan(args.excitation_acceleration) != isnan(args.excitation_amplitude));

    const float excitation_amplitude = //
        !isnan(args.excitation_amplitude)
        ? args.excitation_amplitude
        : HarmonicGenerator::amplitudeNotRounded(requested_frequency, args.excitation_acceleration);

    HarmonicGenerator generator(requested_frequency, excitation_amplitude, args.step_len);

    const float excitation_frequency = generator.getFrequency();
    const float measurement_frequency = excitation_frequency * args.measured_harmonic;

    const float excitation_period = 1 / excitation_frequency;
    const float measurement_period = 1 / measurement_frequency;

    const float measurement_freq_2pi = std::numbers::pi_v<float> * measurement_frequency * 2.f;

    StepDir stepDir(generator);

    const float acceleration = generator.getAcceleration(excitation_frequency);
    PrusaAccelerometer accelerometer;
    if (!is_ok(accelerometer.get_error())) {
        return FrequencyGain3dError { .error = true };
    }

    Accumulator accumulator = {};

    float accelerometer_period_time = 0.f;
    const float accelerometer_sample_period = maybe_calibrate_and_get_accelerometer_sample_period(accelerometer, args.calibrate_accelerometer, progress_hook);
    if (isnan(accelerometer_sample_period)) {
        return FrequencyGain3dError { .error = true };
    }

    uint32_t sample_nr = 0;
    const bool do_delayed_measurement = (args.measurement_cycles != 0);
    const auto measurement_cycles = do_delayed_measurement ? args.measurement_cycles : args.excitation_cycles;
    const uint32_t samples_to_collect = excitation_period * measurement_cycles / accelerometer_sample_period;
    bool enough_samples_collected = false;

    TEMPORARY_AUTO_REPORT_OFF(suspend_auto_report);
#ifdef M958_OUTPUT_SAMPLES
    SERIAL_ECHOLN("Yraw  sinf cosf");
#endif

    constexpr int num_axis = sizeof(PrusaAccelerometer::Acceleration::val) / sizeof(PrusaAccelerometer::Acceleration::val[0]);

    uint32_t step_nr = 0;
    GcodeSuite::reset_stepper_timeout();
    const uint32_t steps_to_do = generator.getStepsPerPeriod() * args.excitation_cycles;
    const uint32_t steps_to_do_max = steps_to_do * 2 + generator.getStepsPerPeriod() + STEP_EVENT_QUEUE_SIZE;

    /// Processes one sample from the accelerometer.
    /// \returns true if there was a sample to process.
    const auto collect_sample = [&](const PrusaAccelerometer::Acceleration &measured_acceleration) {
        metric_record_custom(&accel, " x=%.4f,y=%.4f,z=%.4f", (double)measured_acceleration.val[0], (double)measured_acceleration.val[1], (double)measured_acceleration.val[2]);
        const float accelerometer_time_2pi_measurement_freq = measurement_freq_2pi * accelerometer_period_time;
        const std::complex<float> amplitude = { sinf(accelerometer_time_2pi_measurement_freq), cosf(accelerometer_time_2pi_measurement_freq) };

        for (int axis = 0; axis < num_axis; ++axis) {
            accumulator.val[axis] += amplitude * measured_acceleration.val[axis];
        }

        ++sample_nr;
        enough_samples_collected = (sample_nr >= samples_to_collect);
        accelerometer_period_time += accelerometer_sample_period;
        if (accelerometer_period_time > measurement_period) {
            accelerometer_period_time -= measurement_period;
        }

#ifdef M958_OUTPUT_SAMPLES
        char buff[40];
        snprintf(buff, 40, "%f %f %f\n", static_cast<double>(measured_acceleration.val[1]), static_cast<double>(amplitude[0]), static_cast<double>(amplitude[1]));
        tud_cdc_n_write_str(0, buff);
        tud_cdc_write_flush();
#endif
    };

    accelerometer.clear();

    // Excitation phase (with accelerometer sample collection, if the measurement is not delayed)
    while (
        // Enqueue at least \p steps_to_do
        (step_nr < steps_to_do)

        // Repeat until we have enough samples, if the measurement is not delayed
        || (!do_delayed_measurement && !enough_samples_collected)

        // Always enqueue whole sine waves (do not stop in the middle of the period)
        || (step_nr % generator.getStepsPerPeriod() != 0)

    ) {
        while (is_full()) {
            PrusaAccelerometer::Acceleration measured_acceleration;
            bool got_sample = accelerometer.get_sample(measured_acceleration);
            if (do_delayed_measurement) {
                // If the measurement is delayed, just clear the accelerometer buffer
                accelerometer.clear();

            } else if (!got_sample) {
                // Failed to obtain thie sample, whatevs

            } else if (step_nr <= STEP_EVENT_QUEUE_SIZE) {
                // Start collecting samples only after we're sure that the steppers are processing our excitation movements and something that was in the buffer before that.
                // So discard samples until we've queued enough steps to fill the entire stepper buffer (at which point we can be sure there is nothing remaining).

            } else if (!enough_samples_collected) {
                collect_sample(measured_acceleration);
            }

            // Send the metric only when the step queue is full, to prevent possible movement stall
            metric_record_float(&metric_excite_freq, excitation_frequency);

            if (!got_sample) {
                idle(true, true);
            }
        }

        const StepDir::RetVal step_dir = stepDir.get();
        enqueue_step(step_dir.step_us, step_dir.dir, args.axis_flag);
        ++step_nr;

        if (step_nr > steps_to_do_max) {
            SERIAL_ERROR_MSG("vibrate measure: getting accelerometer samples timed out");
            (void)is_ok(accelerometer.get_error());
            return FrequencyGain3dError { .error = true };
        }
    }

    if (!is_ok(accelerometer.get_error())) {
        return FrequencyGain3dError { .error = true };
    }

    // Possible delayed measurement
    if (do_delayed_measurement) {
        const auto has_steps = []() {
            // Cannot use freertos::CriticalSection here - steppers have higher priority than RTOS-aware interrupts
            CRITICAL_SECTION_START;
            const auto result = PreciseStepping::has_step_events_queued();
            CRITICAL_SECTION_END;
            return result;
        };

        // Wait till all the movement is executed
        while (has_steps()) {
            accelerometer.clear();
            idle(true, true);
        }

        // Then wait for the specified time
        {
            const uint32_t end_time = millis() + excitation_period * args.wait_cycles * 1000.f;
            while (ticks_diff(millis(), end_time) > 0) {
                accelerometer.clear();
                idle(true, true);
            }
        }

        // And then finally do the measurement
        accelerometer.clear();

        while (!enough_samples_collected) {
            PrusaAccelerometer::Acceleration measured_acceleration;
            const bool got_sample = accelerometer.get_sample(measured_acceleration);

            if (got_sample) {
                collect_sample(measured_acceleration);
            } else {
                idle(true, true);
            }
        }
    }

    for (int axis = 0; axis < num_axis; ++axis) {
        accumulator.val[axis] *= 2.;
        accumulator.val[axis] /= (sample_nr + 1);
    }

    const float x_acceleration_amplitude = std::abs(accumulator.val[0]);
    const float y_acceleration_amplitude = std::abs(accumulator.val[1]);
    const float z_acceleration_amplitude = std::abs(accumulator.val[2]);
    const float x_gain = x_acceleration_amplitude / acceleration;
    const float y_gain = y_acceleration_amplitude / acceleration;
    const float z_gain = z_acceleration_amplitude / acceleration;

#ifdef M958_VERBOSE
    SERIAL_ECHO_START();
    SERIAL_ECHOPAIR_F("frequency ", frequency);
    SERIAL_ECHOPAIR_F(" Msampl ", (sample_nr + 1));
    SERIAL_ECHOPAIR_F(" Xsin ", accumulator.val[0][0], 5);
    SERIAL_ECHOPAIR_F(" Xcos ", accumulator.val[0][1], 5);
    SERIAL_ECHOPAIR_F(" Ysin ", accumulator.val[1][0], 5);
    SERIAL_ECHOPAIR_F(" Ycos ", accumulator.val[1][1], 5);
    SERIAL_ECHOPAIR_F(" Zsin ", accumulator.val[2][0], 5);
    SERIAL_ECHOPAIR_F(" Zcos ", accumulator.val[2][1], 5);
    SERIAL_ECHOPAIR_F(" X ", x_acceleration_amplitude, 5);
    SERIAL_ECHOPAIR_F(" Y ", y_acceleration_amplitude, 5);
    SERIAL_ECHOLNPAIR_F(" Z ", z_acceleration_amplitude, 5);
#else
    SERIAL_ECHO(excitation_frequency);
    if (args.klipper_mode) {
        SERIAL_ECHOPAIR_F(",", sq(x_gain), 5);
        SERIAL_ECHOPAIR_F(",", sq(y_gain), 5);
        SERIAL_ECHOPAIR_F(",", sq(z_gain), 5);
        SERIAL_ECHOLNPAIR_F(",", sq(x_gain) + sq(y_gain) + sq(z_gain), 5);
    } else {
        SERIAL_ECHOPAIR_F(" ", measurement_frequency);
        SERIAL_ECHOPAIR_F(" ", acceleration);
        SERIAL_ECHOPAIR_F(" ", x_acceleration_amplitude, 5);
        SERIAL_ECHOPAIR_F(" ", y_acceleration_amplitude, 5);
        SERIAL_ECHOPAIR_F(" ", z_acceleration_amplitude, 5);
        SERIAL_ECHOPAIR_F(" ", x_gain, 5);
        SERIAL_ECHOPAIR_F(" ", y_gain, 5);
        SERIAL_ECHOLNPAIR_F(" ", z_gain, 5);
    }
#endif

    AxisEnum logical_axis = get_logical_axis(args.axis_flag);
    metric_record_custom(&metric_freq_gain, " a=%d,f=%.1f,x=%.4f,y=%.4f,z=%.4f",
        logical_axis, excitation_frequency, x_gain, y_gain, z_gain);

    return { { excitation_frequency, { x_gain, y_gain, z_gain } }, false };
}

static FrequencyGain3dError vibrate_measure(const VibrateMeasureParams &args, float frequency) {
    const auto progress_hook = [](auto) {
        idle(true, true);
        return true;
    };
    return vibrate_measure(args, frequency, progress_hook);
}

static FrequencyGain3dError vibrate_measure_repeat(const VibrateMeasureParams &args, float frequency) {
    constexpr int max_attempts = 3;
    FrequencyGain3dError ret_val;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        ret_val = vibrate_measure(args, frequency);
        if (!ret_val.error) {
            break;
        }
    }
    if (ret_val.error) {
        SERIAL_ERROR_MSG("maximum attempts exhausted");
    }
    return ret_val;
}

/**
 * @brief Excite harmonic vibration and measure amplitude if there is an accelerometer
 *
 * Repeat if there is an error up to max_attempts times.
 *
 * @see GcodeSuite::M958() for parameter description
 *
 * @param axis_flag StepEventFlag bit field
 * STEP_EVENT_FLAG_STEP_* is set for all the motors which should vibrate together
 * STEP_EVENT_FLAG_*_DIR encodes initial phase for each motor
 * @param klipper_mode
 * @param frequency_requested
 * @param acceleration_requested
 * @param cycles
 * @param calibrate_accelerometer
 * @return Frequency and gain measured on each axis if there is accelerometer
 */
FrequencyGain3dError vibrate_measure_repeat(const VibrateMeasureParams &args, float frequency, const SamplePeriodProgressHook &progress_hook) {
    constexpr int max_attempts = 3;
    FrequencyGain3dError ret_val;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        ret_val = vibrate_measure(args, frequency, progress_hook);
        if (!ret_val.error) {
            break;
        }
    }
    if (ret_val.error) {
        SERIAL_ERROR_MSG("maximum attempts exhausted");
    }
    return ret_val;
}

/**
 * @brief Parse motor codes, directions and enable motors accordingly
 *
 * @return step and direction flags - see StepEventFlag
 */
static StepEventFlag_t setup_axis() {
    // As we modify driver's microsteps directly, phase stepping needs to be off
    phase_stepping::assert_disabled();

    // enable all axes to have the same state as printing
    enable_all_steppers();

    StepEventFlag_t axis_flag = 0;
    LOOP_XYZ(i) {
        const char axis_code = axis_codes[i];
        if (parser.seen(axis_code)) {
            axis_flag |= (StepEventFlag::STEP_EVENT_FLAG_STEP_X << i);
            // Old Core XY support for backwards compatibility. It required
            // specifying both axes and used a -1 value to distinguish X and Y
            if (parser.seenval(axis_code) && parser.value_long() == -1) {
                axis_flag |= (StepEventFlag::STEP_EVENT_FLAG_X_DIR << i);
            }
        }
    }

    if (axis_flag == 0) {
        // no axis requested, assume X
        axis_flag = StepEventFlag::STEP_EVENT_FLAG_STEP_X;
    }

#if ENABLED(COREXY)
    // For Core XY, X and Y are actually A and B motors, so we need to use both
    // and for Y axis reverse the B direction
    if (axis_flag == StepEventFlag::STEP_EVENT_FLAG_STEP_X) {
        axis_flag |= StepEventFlag::STEP_EVENT_FLAG_STEP_Y;
    } else if (axis_flag == StepEventFlag::STEP_EVENT_FLAG_STEP_Y) {
        axis_flag |= StepEventFlag::STEP_EVENT_FLAG_STEP_X | StepEventFlag::STEP_EVENT_FLAG_Y_DIR;
    }
#endif

    if (axis_flag & STEP_EVENT_FLAG_STEP_X) {
        stepper_microsteps(X_AXIS, 128);
    }
    if (axis_flag & STEP_EVENT_FLAG_STEP_Y) {
        stepper_microsteps(Y_AXIS, 128);
    }
    if (axis_flag & STEP_EVENT_FLAG_STEP_Z) {
        stepper_microsteps(Z_AXIS, 128);
    }

    return axis_flag;
}

/**
 * @brief Get current step length
 *
 * Compute step length based on kinematic type, default steps per mm, default microstep resolution
 * current microstep resolution and number of active motors.
 *
 * @param axis_flag All active motors when generating harmonic vibrations
 * @param orig_mres Original microstep resolution (matching planner.mm_per_step)
 * @return step length in meters
 */
float get_step_len(StepEventFlag_t axis_flag, const uint16_t orig_mres[]) {
    constexpr float meters_in_mm = 0.001f;

    // index motors
    uint8_t motor_cnt = 0;
    AxisEnum motor_idx[3];
    LOOP_XYZ(i) {
        if (axis_flag & (STEP_EVENT_FLAG_STEP_X << i)) {
            motor_idx[motor_cnt++] = (AxisEnum)i;
        }
    }
    if (motor_cnt < 1) {
        SERIAL_ECHOLN("error: at least one motor needs to be enabled");
        return NAN;
    }

    // calculate the step_len for the first axis
    const AxisEnum first_axis = motor_idx[0];
    const float mm_per_step = planner.mm_per_step[first_axis] * (float)(orig_mres[first_axis]) / (float)stepper_microsteps(first_axis);
    const float step_len = mm_per_step * meters_in_mm; // in meters

    // check consistency for other axes
    for (uint8_t i = 1; i != motor_cnt; ++i) {
        const AxisEnum this_axis = motor_idx[i];
        const float this_mm_per_step = planner.mm_per_step[this_axis] * (float)(orig_mres[this_axis]) / (float)stepper_microsteps(this_axis);
        if (mm_per_step != this_mm_per_step) {
            SERIAL_ECHOLN("error: same step resolution expected on all excited axes");
            return NAN;
        }
    }

#if IS_CARTESIAN
    // return correct step length
    if ((motor_cnt == 1 && (motor_idx[0] == X_AXIS || motor_idx[0] == Y_AXIS))
        || (motor_cnt == 2 && motor_idx[0] == X_AXIS && motor_idx[1] == Y_AXIS)) {
        // X, Y, XY
    #if IS_CORE
        #if CORE_IS_XY
        switch (motor_cnt) {
        case 1:
            // diagonal
            return sqrt(2.f) / 2.f * step_len;
        case 2:
            // orthogonal
            return step_len;
        }
        #else
            #error "Not implemented."
        #endif
    #else
        switch (motor_cnt) {
        case 1:
            // orthogonal
            return step_len;
        case 2:
            // diagonal
            return sqrt(2.f) * step_len;
        }
    #endif
    } else if (motor_cnt == 1) {
        // single motor (not XY)
        return step_len;
    }
#else
    #error "Not implemented."
#endif

    SERIAL_ECHOLN("error: unsupported configuration");
    return NAN;
}

static bool is_ok(PrusaAccelerometer::Error error) {
    switch (error) {
    case PrusaAccelerometer::Error::none:
        return true;
    case PrusaAccelerometer::Error::communication:
        SERIAL_ERROR_MSG("accelerometer communication");
        break;
#if HAS_REMOTE_ACCELEROMETER()
    case PrusaAccelerometer::Error::no_active_tool:
        SERIAL_ERROR_MSG("no active tool");
        break;
    case PrusaAccelerometer::Error::busy:
        SERIAL_ERROR_MSG("busy");
        break;
#endif
    case PrusaAccelerometer::Error::overflow_sensor:
        SERIAL_ERROR_MSG("sample overrun on accelerometer sensor");
        break;
#if HAS_REMOTE_ACCELEROMETER()
    case PrusaAccelerometer::Error::overflow_buddy:
        SERIAL_ERROR_MSG("sample missed on buddy");
        break;
    case PrusaAccelerometer::Error::overflow_dwarf:
        SERIAL_ERROR_MSG("sample missed on dwarf");
        break;
    case PrusaAccelerometer::Error::overflow_possible:
        SERIAL_ERROR_MSG("sample possibly lost in transfer");
        break;
#endif
    }
    return false;
}

/** \addtogroup G-Codes
 * @{
 */

/**
 * @brief Excite harmonic vibration
 *
 * - X<direction> Vibrate with X motor, start in direction 1 or -1
 * - Y<direction> Vibrate with Y motor, start in direction 1 or -1
 * - Z<direction> Vibrate with Z motor, start in direction 1 or -1
 * - F<Hz>     Frequency
 * - A<mm/s-2> Acceleration
 * - N<cycles> Number of full periods at desired frequency.
 *             In case there is no accelerometer measurement,
 *             exact number of periods is generated,
 *             in case there is accelerometer, it is
 *             number of periods of active measurement and
 *             some extra cycles can be generated.
 * - C         Calibrate accelerometer sample rate
 * - K         Klipper compatible report
 * - I<n>      Which harmonic frequency to measure
 */
void GcodeSuite::M958() {
    // phstep needs to be off _before_ getting the current ustep resolution
    phase_stepping::EnsureDisabled phaseSteppingDisabler;
    MicrostepRestorer microstepRestorer;

    PrusaAccelerometer accelerometer;
    VibrateMeasureParams args {
        .excitation_acceleration = 2.5f,
        .excitation_cycles = 50,
        .klipper_mode = parser.seen('K'),
        .calibrate_accelerometer = parser.seen('C'),
        .axis_flag = setup_axis(), // modifies mres as a side-effect
    };
    float frequency = 35;

    if (parser.seenval('F')) {
        frequency = abs(parser.value_float());
    }
    if (parser.seenval('A')) {
        args.excitation_acceleration = abs(parser.value_float()) * 0.001f;
    }
    if (parser.seenval('N')) {
        args.excitation_cycles = parser.value_ulong();
    }
    if (parser.seenval('I')) {
        args.measured_harmonic = parser.value_ulong();
    }

    if (!args.setup(microstepRestorer)) {
        return;
    }

    serial_echo_header(args.klipper_mode);
    vibrate_measure(args, frequency);
}

/** @}*/

static constexpr float epsilon = 0.01f;

/**
 * @brief ZV shaper tune, naive approach
 *
 * Sadly damping ratio computation is very probably not right here. In theory we could identify damping ratio from gain
 * at resonant frequency if the system would be excited by sine wave force.
 * But in reality we are exciting the system by sine wave displacement. We for sure can not tell, if the force is still sine wave
 * and what is the force - the force depends on motor load angle and belt stiffness and we don't know it.
 */
static void naive_zv_tune(VibrateMeasureParams &args, const VibrateMeasureRange &range) {
    FrequencyGain maxFrequencyGain = { 0.f, 0.f };
    const AxisEnum logicalAxis = get_logical_axis(args.axis_flag);
    if (logicalAxis == NO_AXIS_ENUM) {
        SERIAL_ECHOLN("error: not moving along one logical axis");
        return;
    }

    serial_echo_header(args.klipper_mode);

    for (float frequency = range.start_frequency; frequency <= range.end_frequency + epsilon; frequency += range.frequency_increment) {
        FrequencyGain3dError frequencyGain3dError = vibrate_measure_repeat(args, frequency);
        args.calibrate_accelerometer = false;
        if (frequencyGain3dError.error) {
            return;
        }
        FrequencyGain frequencyGain = { frequencyGain3dError.frequencyGain3d.frequency, frequencyGain3dError.frequencyGain3d.gain[logicalAxis] };
        if (frequencyGain.gain > maxFrequencyGain.gain) {
            maxFrequencyGain = frequencyGain;
        }
    }
    const float damping_ratio = get_zv_shaper_damping_ratio(maxFrequencyGain.gain);

    SERIAL_ECHOPAIR_F("ZV tune: Maximum resonant gain: ", maxFrequencyGain.gain);
    SERIAL_ECHOPAIR_F(" at frequency: ", maxFrequencyGain.frequency);
    SERIAL_ECHOLNPAIR_F(" damping ratio: ", damping_ratio, 5);

    input_shaper::AxisConfig axis_config {
        .type = input_shaper::Type::zv,
        .frequency = maxFrequencyGain.frequency,
        .damping_ratio = damping_ratio,
        .vibration_reduction = 0.f,
    };

    input_shaper::set_axis_config(logicalAxis, axis_config);
}

static float limit_end_frequency(const float start_frequency, float end_frequency, const float frequency_increment, const size_t max_samples) {
    const size_t requested_samples = (end_frequency - start_frequency + epsilon) / frequency_increment + 1;
    if (requested_samples > max_samples) {
        end_frequency = start_frequency + (max_samples - 1) * frequency_increment;
    }
    return end_frequency;
}

static double get_inv_D(const input_shaper::Shaper &shaper) {
    double d = 0.;
    for (int i = 0; i < shaper.num_pulses; ++i) {
        d += shaper.a[i];
    }
    return 1. / d;
}

/**
 * @brief Get vibration reduction
 *
 * called _estimate_shaper in klipper source
 *
 * @param shaper
 * @param system_damping_ratio
 * @param frequency
 * @return Vibration reduction for system with system_damping_ratio at frequency.
 */
static double vibration_reduction(const input_shaper::Shaper &shaper, float system_damping_ratio, float frequency) {
    const double inv_D = get_inv_D(shaper);
    const double omega = 2. * std::numbers::pi_v<double> * frequency;
    const double damping = system_damping_ratio * omega;
    const double omega_d = omega * sqrt(1. - sq(system_damping_ratio));

    double s = 0.;
    double c = 0.;

    for (int i = 0; i < shaper.num_pulses; ++i) {
        const double w = shaper.a[i] * exp(-damping * (shaper.t[shaper.num_pulses - 1] - shaper.t[i]));
        s += w * sin(omega_d * shaper.t[i]);
        c += w * cos(omega_d * shaper.t[i]);
    }
    return (sqrt(sq(s) + sq(c)) * inv_D);
}

/**
 * @brief Get remaining vibrations
 *
 * called _estimate_remaining_vibrations in klipper source
 *
 * @param shaper
 * @param system_damping_ratio
 * @param psd power spectrum density
 *
 * @return remaining vibrations
 */
static float remaining_vibrations(const input_shaper::Shaper &shaper, float system_damping_ratio, const Spectrum &psd, float default_vibration_reduction) {
    float vibr_threshold = psd.max() / default_vibration_reduction;
    float remaining_vibrations_sum = 0.f;
    float all_vibrations_sum = 0.f;
    for (size_t i = 0; i < psd.size(); ++i) {
        const FrequencyGain fg = psd.get(i);
        all_vibrations_sum += max(fg.gain, 0.f);
        const float vibration = max(fg.gain * static_cast<float>(vibration_reduction(shaper, system_damping_ratio, fg.frequency)) - vibr_threshold, 0.f);
        remaining_vibrations_sum += vibration;
    }
    return (remaining_vibrations_sum / all_vibrations_sum);
}

/**
 * @brief Get shaper smoothing
 *
 * Called _get_shaper_smoothing in klipper source
 *
 * @param shaper
 * @return smoothing
 */
static float smoothing(const input_shaper::Shaper &shaper) {
    constexpr float accel = 5000.f;
    constexpr float scv = 5.f;
    constexpr float half_accel = accel / 2.f;

    const double inv_D = get_inv_D(shaper);

    double ts = 0.;
    for (int i = 0; i < shaper.num_pulses; ++i) {
        ts += shaper.a[i] * shaper.t[i];
    }
    ts *= inv_D;

    double offset_90 = 0.;
    double offset_180 = 0.;

    for (int i = 0; i < shaper.num_pulses; ++i) {
        if (shaper.t[i] >= ts) {
            /// Calculate offset for one of the axes
            offset_90 += shaper.a[i] * (scv + half_accel * (shaper.t[i] - ts)) * (shaper.t[i] - ts);
        }
        offset_180 += shaper.a[i] * half_accel * sq(shaper.t[i] - ts);
    }
    offset_90 *= (inv_D * sqrt(2.));
    offset_180 *= inv_D;

    return max(offset_90, offset_180);
}

namespace {
enum class Action {
    first,
    /// Find lowest vibrs, reverse order, return when maximum smoothing exceeded
    find_best_result = first,
    /// Try to find an 'optimal' shapper configuration: the one that is not
    /// much worse than the 'best' one, but gives much less smoothing
    select,
    last = select,
};
Action &operator++(Action &action) {
    using IntType = typename std::underlying_type<Action>::type;
    action = static_cast<Action>(static_cast<IntType>(action) + 1);
    return action;
}
} // anonymous namespace

struct Shaper_result {
    float frequency;
    float score;
    float smoothing;
};
static Shaper_result fit_shaper(const FindBestShaperProgressHook &progress_hook, input_shaper::Type type, const Spectrum &psd, const Action final_action, input_shaper::AxisConfig default_config) {
    constexpr float start_frequency = 5.f;
    constexpr float end_frequency = 150.f;
    constexpr float frequency_step = .2f;
    constexpr float max_smoothing = std::numeric_limits<float>::max();

    const float default_damping_ratio = default_config.damping_ratio;
    const float default_vibration_reduction = default_config.vibration_reduction;

    struct Result {
        float frequency;
        float score;
        float smoothing;
        float vibrs;
    };

    Result best_result = { 0.f, 0.f, 0.f, std::numeric_limits<float>::max() };
    Result selected_result = { 0.f, 0.f, 0.f, std::numeric_limits<float>::max() };

    for (Action action = Action::first; action <= final_action; ++action) {
        for (float frequency = end_frequency; frequency >= start_frequency - epsilon; frequency -= frequency_step) {
            const float progress_ratio = (end_frequency - frequency) / (end_frequency - start_frequency);
            if (!progress_hook(type, progress_ratio)) {
                return {};
            }
            input_shaper::Shaper shaper = input_shaper::get(default_damping_ratio, frequency, default_vibration_reduction, type);
            const float shaper_smoothing = smoothing(shaper);
            if (Action::find_best_result == action && best_result.frequency && shaper_smoothing > max_smoothing) {
                Shaper_result result = { .frequency = best_result.frequency, .score = best_result.score, .smoothing = best_result.smoothing };
                return result;
            }
            /// Exact damping ratio of the printer is unknown, pessimizing
            /// remaining vibrations over possible damping values
            float shaper_vibrations = 0.f;
            for (double damping_ratio = 0.05; damping_ratio <= 0.15 + epsilon; damping_ratio += 0.05) {
                double vibrations = remaining_vibrations(shaper, damping_ratio, psd, default_vibration_reduction);
                if (vibrations > shaper_vibrations) {
                    shaper_vibrations = vibrations;
                }
                if (!progress_hook(type, progress_ratio)) {
                    return {};
                }
            }
            /// todo max_accel = self.find_shaper_max_accel(shaper) (not needed to fit filter)

            /// The score trying to minimize vibrations, but also accounting
            /// the growth of smoothing. The formula itself does not have any
            /// special meaning, it simply shows good results on real user data
            const float shaper_score = shaper_smoothing * (pow(shaper_vibrations, 1.5) + shaper_vibrations * .2 + .01);

            if (Action::find_best_result == action && shaper_vibrations < best_result.vibrs) {
                Result result = { .frequency = frequency, .score = shaper_score, .smoothing = shaper_smoothing, .vibrs = shaper_vibrations };
                best_result = result;
                selected_result = best_result;
            }

            if (Action::select == action && (shaper_vibrations < (best_result.vibrs * 1.1f)) && shaper_score < selected_result.score) {
                Result result = { .frequency = frequency, .score = shaper_score, .smoothing = shaper_smoothing, .vibrs = shaper_vibrations };
                selected_result = result;
            }
        }
        SERIAL_ECHO_START();
        SERIAL_ECHOPAIR("IS filter: ", to_string(type));
        switch (action) {
        case Action::find_best_result:
            SERIAL_ECHOPAIR(" lowest vibration freq: ", selected_result.frequency);
            break;
        case Action::select:
            SERIAL_ECHOPAIR(" selected freq: ", selected_result.frequency);
            break;
        }
        SERIAL_ECHO(" score: ");
        SERIAL_PRINT(selected_result.score, 6);
        SERIAL_ECHO(" remaining vibrations: ");
        SERIAL_PRINT(selected_result.vibrs, 8);
        SERIAL_ECHO(" smoothing: ");
        SERIAL_PRINTLN(selected_result.smoothing, 4);
    }

    Shaper_result shaper_result = { .frequency = selected_result.frequency, .score = selected_result.score, .smoothing = selected_result.smoothing };
    return shaper_result;
}

static input_shaper::AxisConfig find_best_shaper(const FindBestShaperProgressHook &progress_hook, const Spectrum &psd, const Action final_action, input_shaper::AxisConfig default_config) {
    struct Best_score {
        Shaper_result result;
        input_shaper::Type type;
    };
    std::optional<Best_score> best_shaper;

    for (input_shaper::Type shaper_type = input_shaper::Type::first; shaper_type <= input_shaper::Type::last; ++shaper_type) {
        if (shaper_type == input_shaper::Type::null || !input_shaper::enabled_filters[ftrstd::to_underlying(shaper_type)]) {
            continue;
        }

        if (!progress_hook(shaper_type, 0.0f)) {
            break;
        }

        Shaper_result shaper = fit_shaper(progress_hook, shaper_type, psd, final_action, default_config);

        if (
            !best_shaper.has_value() //
            || shaper.score * 1.2f < best_shaper->result.score
            || ((shaper.score * 1.05f < best_shaper->result.score) && (shaper.smoothing * 1.1f < best_shaper->result.smoothing)) //
        ) {
            best_shaper = {
                .result = shaper,
                .type = shaper_type,
            };
        }
    }

    return input_shaper::AxisConfig {
        .type = best_shaper ? best_shaper->type : input_shaper::Type::null,
        .frequency = best_shaper ? best_shaper->result.frequency : 0,
        .damping_ratio = default_config.damping_ratio,
        .vibration_reduction = default_config.vibration_reduction,
    };
}

static input_shaper::AxisConfig find_best_shaper(const Spectrum &psd, const Action final_action, input_shaper::AxisConfig default_config) {
    const auto progress_hook = [](input_shaper::Type, float) {
        idle(true, true);
        return true;
    };
    return find_best_shaper(progress_hook, psd, final_action, default_config);
}

input_shaper::AxisConfig find_best_shaper(const FindBestShaperProgressHook &progress_hook, const Spectrum &psd, input_shaper::AxisConfig default_config) {
    return find_best_shaper(progress_hook, psd, Action::find_best_result, default_config);
}

/**
 * @brief
 *
 * To save memory we assume reached frequency was equal to requested, so frequency returned by vibrate_measure() is discarded.
 */
static void klipper_tune(VibrateMeasureParams &args, VibrateMeasureRange range, bool subtract_excitation) {
    // Power spectrum density
    FixedLengthSpectrum<146> psd(range.start_frequency, range.frequency_increment);
    range.end_frequency = limit_end_frequency(range.start_frequency, range.end_frequency, range.frequency_increment, psd.max_size());
    const AxisEnum logicalAxis = get_logical_axis(args.axis_flag);
    if (logicalAxis == NO_AXIS_ENUM) {
        SERIAL_ECHOLN("error: not moving along one logical axis");
        return;
    }

    serial_echo_header(args.klipper_mode);

    for (float frequency = range.start_frequency; frequency <= range.end_frequency + epsilon; frequency += range.frequency_increment) {
        FrequencyGain3dError frequencyGain3dError = vibrate_measure_repeat(args, frequency);
        args.calibrate_accelerometer = false;
        if (frequencyGain3dError.error) {
            return;
        }
        if (subtract_excitation) {
            frequencyGain3dError.frequencyGain3d.gain[logicalAxis] = max(frequencyGain3dError.frequencyGain3d.gain[logicalAxis] - 1.f, 0.f);
        }
        const float psd_xyz = sq(frequencyGain3dError.frequencyGain3d.gain[0]) + sq(frequencyGain3dError.frequencyGain3d.gain[1]) + sq(frequencyGain3dError.frequencyGain3d.gain[2]);
        psd.put(psd_xyz);
    }

    TEMPORARY_AUTO_REPORT_OFF(suspend_auto_report);

    if (subtract_excitation) {
        SERIAL_ECHOLN("Excitation subtracted power spectrum density");
        SERIAL_ECHOLN("freq,psd_xyz");
        for (size_t i = 0; i < psd.size(); ++i) {
            FrequencyGain fg = psd.get(i);
            SERIAL_ECHO(fg.frequency);
            SERIAL_ECHOLNPAIR_F(",", fg.gain, 5);
        }
    }

    const Action final_action = subtract_excitation ? Action::find_best_result : Action::last;
    input_shaper::AxisConfig axis_config = find_best_shaper(psd, final_action, input_shaper::axis_defaults[logicalAxis]);
    input_shaper::set_axis_config(logicalAxis, axis_config);
}

MicrostepRestorer::MicrostepRestorer() {
    LOOP_XYZ(i) {
        state[i] = stepper_microsteps((AxisEnum)i);
    }
}
MicrostepRestorer::~MicrostepRestorer() {
    const auto has_steps = []() {
        // Cannot use freertos::CriticalSection here - steppers have higher priority than RTOS-aware interrupts
        CRITICAL_SECTION_START;
        const auto result = PreciseStepping::has_step_events_queued();
        CRITICAL_SECTION_END;
        return result;
    };
    while (has_steps()) {
        idle(true, true);
    }
    LOOP_XYZ(i) {
        stepper_microsteps((AxisEnum)i, state[i]);
    }
}

/** \addtogroup G-Codes
 * @{
 */

/**
 * @brief Tune input shaper
 *
 * - X<direction> Vibrate with X motor, start in direction 1 or -1
 * - Y<direction> Vibrate with Y motor, start in direction 1 or -1
 * - Z<direction> Vibrate with Z motor, start in direction 1 or -1
 * - K           select Klipper tune algorithm
 * - KM          select Klipper Marek modified tune algorithm
 * - F<Hz>       Start frequency
 * - G<Hz>       End frequency
 * - H<Hz>       Frequency step
 * - A<mm/s-2>   Acceleration
 * - D           Don't home and move to bed center before calibrating
 * - N<cycles>   Number of excitation signal periods
 *               of active measurement.
 * - W           Write the detected calibration to EEPROM
 * - I<n>        Which harmonic frequency to measure
 */
void GcodeSuite::M959() {
    SERIAL_ECHO_START();
    SERIAL_ECHOLNPAIR("Running: ", parser.get_command());

    if (!parser.seen('D')) {
        GcodeSuite::G28_no_parser(false, true, NAN, false, true, true, true);

        current_position[X_AXIS] = X_BED_SIZE / 2;
        current_position[Y_AXIS] = Y_BED_SIZE / 2;
        current_position[Z_AXIS] = 20;
        line_to_current_position(HOMING_FEEDRATE_XY);
        planner.synchronize();
    }

    // phstep needs to be off _before_ getting the current ustep resolution
    phase_stepping::EnsureDisabled phaseSteppingDisabler;
    MicrostepRestorer microstep_restorer;
    PrusaAccelerometer accelerometer;

    VibrateMeasureParams args {
        .excitation_acceleration = 2.5f,
        .excitation_cycles = 50,
        .klipper_mode = parser.seen('K'),
        .calibrate_accelerometer = true,
        .axis_flag = setup_axis(), // modifies mres as a side-effect
    };
    VibrateMeasureRange range {
        .start_frequency = 5,
        .end_frequency = 150,
        .frequency_increment = 1,
    };

    if (parser.seenval('F')) {
        range.start_frequency = abs(parser.value_float());
    }
    if (parser.seenval('G')) {
        range.end_frequency = abs(parser.value_float());
    }
    if (parser.seenval('H')) {
        range.frequency_increment = abs(parser.value_float());
    }
    if (parser.seenval('A')) {
        args.excitation_acceleration = abs(parser.value_float()) * 0.001f;
    }
    if (parser.seenval('N')) {
        args.excitation_cycles = parser.value_ulong();
    }
    if (parser.seenval('I')) {
        args.measured_harmonic = parser.value_ulong();
    }

    if (!args.setup(microstep_restorer)) {
        return;
    }

    if (args.klipper_mode) {
        klipper_tune(args, range, parser.seen('M'));
    } else {
        naive_zv_tune(args, range);
    }

    const AxisEnum logical_axis = get_logical_axis(args.axis_flag);
    auto axis_config = *input_shaper::current_config().axis[logical_axis];

    SERIAL_ECHO_START();
    SERIAL_ECHOPAIR("IS calibrated for axis: ", axis_codes[logical_axis]);
    SERIAL_ECHOPAIR(" filter: ", to_string(axis_config.type));
    SERIAL_ECHOPAIR_F(" frequency: ", axis_config.frequency);

    if (parser.seen('W')) {
        SERIAL_ECHOLN(", saving");
        config_store().set_input_shaper_config(input_shaper::current_config());
    } else {
        SERIAL_ECHOLN("");
    }
}

/** @}*/
