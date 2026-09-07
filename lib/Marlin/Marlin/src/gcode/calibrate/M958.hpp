#pragma once

#include <cstddef>
#include <expected>

#include <option/has_local_accelerometer.h>
#include <option/has_remote_accelerometer.h>
#include "Marlin/src/core/types.h"
#include "Marlin/src/module/prusa/accelerometer.h"
#include "Marlin/src/feature/precise_stepping/common.hpp"
#include "Marlin/src/feature/input_shaper/input_shaper_config.hpp"
#include <inplace_function.hpp>
#include <freertos/critical_section.hpp>

static_assert(HAS_LOCAL_ACCELEROMETER() || HAS_REMOTE_ACCELEROMETER());

namespace vibrate_measure {

enum class Error {
    aborted,
    failed,
};

template <typename T>
using Result = std::expected<T, Error>;

struct FrequencyGain {
    float frequency;
    float gain;
};

class Spectrum {
public:
    virtual ~Spectrum() = default;

    virtual float max() const = 0;
    virtual size_t size() const = 0;
    virtual FrequencyGain get(size_t index) const = 0;
};

class MicrostepRestorer {
private:
    std::array<uint16_t, 3> state;

public:
    MicrostepRestorer();
    ~MicrostepRestorer();

    const uint16_t *saved_mres() const { return state.data(); }
};

/// \param progress is in range 0-1
using SamplePeriodProgressHook = stdext::inplace_function<Result<void>(float progress)>;

Result<float> maybe_calibrate_and_get_accelerometer_sample_period(PrusaAccelerometer &accelerometer, bool calibrate_accelerometer, const SamplePeriodProgressHook &progress_hook);

Result<float> get_accelerometer_sample_period(const SamplePeriodProgressHook &progress_hook, PrusaAccelerometer &accelerometer);

[[nodiscard]] StepEventFlag_t setup_steppers(StepEventFlag_t axis_flag);

float get_step_len(StepEventFlag_t axis_flag, const uint16_t orig_mres[]);

struct MeasureParams {
    /// How much we're exciting the vibrations, in m/s^2.
    float excitation_acceleration = NAN;

    /// How much we're exciting the vibrations, in meters.
    /// Alternative to using \p excitation_acceleration.
    float excitation_amplitude = NAN;

    /// Configured automatically in setup()
    float step_len = NAN;

    /// How many excitation cycles we should (1/excitation_frequency) do.
    /// If \p measurement_cycles == 0, the measuring is done for this duration as well.
    uint32_t excitation_cycles;

    /// How many cycles (1/excitation_frequency) we should wait before initiating the measurement.
    /// Only used if \p measurement_cycles != 0.
    uint32_t wait_cycles = 0;

    /// If set, the measurement will be done \p wait_cycles after excitation. Otherwise, the measurement is done together with the excitation.
    /// For how many cycles (1/excitation_frequency) we should measure
    uint32_t measurement_cycles = 0;

    bool klipper_mode;
    bool calibrate_accelerometer;
    StepEventFlag_t axis_flag;

    /// Which harmonic frequency to measure
    uint16_t measured_harmonic = 1;

    /// \returns false on failure
    bool setup(const MicrostepRestorer &microstep_restorer);
};

struct MeasureRange {
    float start_frequency;
    float end_frequency;
    float frequency_increment;
};

/// Single point in amplitude frequency response
struct ResponseSample {
    float excitation_frequency;
    xyz_float_t gain;
    xyz_float_t amplitude;

    constexpr float gain_square() const {
        return sq(gain[0]) + sq(gain[1]) + sq(gain[2]);
    }
};

struct ProgressHookParams {
    enum class Phase {
        /// Calibrating accelerometer phase
        calibrating,

        /// The actual measuring.
        measuring,
    };

    /// Phase of \p measure. Phases have separate progress reporting.
    Phase phase;

    /// Progress (0-1) withing the \p phase
    float progress;
};

using ProgressHook = stdext::inplace_function<Result<void>(const ProgressHookParams &params)>;

Result<ResponseSample> measure_repeat(const MeasureParams &args, float frequency, const ProgressHook &progress_hook);

/// Same as \p measure_repeat, but does not retry on failure.
Result<ResponseSample> measure(const MeasureParams &args, float frequency, const ProgressHook &progress_hook);

using FindBestShaperProgressHook = stdext::inplace_function<Result<void>(input_shaper::Type checked_type, float progress)>;

input_shaper::AxisConfig find_best_shaper(const FindBestShaperProgressHook &progress_hook, const Spectrum &psd, input_shaper::AxisConfig default_config);

/// Something that just vibrates.
///
/// Unlike \p measure, this just vibrates. And it doesn't need an accelerometer.
///
/// Fill the structure with requested data, call `setup`. After that, each call
/// to step queues one period of the vibration. The parameters can be changed
/// between calls to step.
struct Vibrate {
    float frequency;

    /// How much we're exciting the vibrations, in m/s^2.
    float excitation_acceleration = NAN;

    /// Configured automatically in setup()
    float step_len = NAN;

    StepEventFlag_t axis_flag = 0;

    /// One-time setup.
    bool setup(const MicrostepRestorer &microstep_restorer);

    /// Queues one period of the vibration.
    ///
    /// Parameters can be changed in between.
    void step();
};

} // namespace vibrate_measure
