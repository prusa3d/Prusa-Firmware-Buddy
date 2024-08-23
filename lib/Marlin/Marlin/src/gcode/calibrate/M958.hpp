#pragma once

#include <cstddef>
#include <limits>
#include <optional>

#include <option/has_local_accelerometer.h>
#include <option/has_remote_accelerometer.h>
#include "Marlin/src/core/types.h"
#include "Marlin/src/module/stepper/trinamic.h"
#include "Marlin/src/module/prusa/accelerometer.h"
#include "Marlin/src/feature/input_shaper/input_shaper_config.hpp"
#include <inplace_function.hpp>
#include <freertos/critical_section.hpp>

static_assert(HAS_LOCAL_ACCELEROMETER() || HAS_REMOTE_ACCELEROMETER());

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

/// \returns false if the measurement should be aborted
using SamplePeriodProgressHook = stdext::inplace_function<bool(float progress)>;

float maybe_calibrate_and_get_accelerometer_sample_period(PrusaAccelerometer &accelerometer, bool calibrate_accelerometer, const SamplePeriodProgressHook &progress_hook);

float get_accelerometer_sample_period(const SamplePeriodProgressHook &progress_hook, PrusaAccelerometer &accelerometer);

float get_step_len(StepEventFlag_t axis_flag, const uint16_t orig_mres[]);

struct VibrateMeasureParams {
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

struct VibrateMeasureRange {
    float start_frequency;
    float end_frequency;
    float frequency_increment;
};

struct VibrateMeasureResult {
    float excitation_frequency;
    xyz_float_t gain;
    xyz_float_t amplitude;

    constexpr float gain_square() const {
        return sq(gain[0]) + sq(gain[1]) + sq(gain[2]);
    }
};

std::optional<VibrateMeasureResult> vibrate_measure_repeat(const VibrateMeasureParams &args, float frequency, const SamplePeriodProgressHook &progress_hook);

/// \returns false if the measurement should be aborted
using FindBestShaperProgressHook = stdext::inplace_function<bool(input_shaper::Type checked_type, float progress)>;

input_shaper::AxisConfig find_best_shaper(const FindBestShaperProgressHook &progress_hook, const Spectrum &psd, input_shaper::AxisConfig default_config);
