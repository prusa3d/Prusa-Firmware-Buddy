#pragma once

#include <cstddef>
#include <limits>
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

struct FrequencyGain3d {
    float frequency;
    float gain[3];

    constexpr float get_square() const {
        return sq(gain[0]) + sq(gain[1]) + sq(gain[2]);
    }
};

struct FrequencyGain3dError {
    FrequencyGain3d frequencyGain3d;
    bool error;
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

float get_accelerometer_sample_period(const SamplePeriodProgressHook &progress_hook, PrusaAccelerometer &accelerometer);

float get_step_len(StepEventFlag_t axis_flag, const uint16_t orig_mres[]);

struct VibrateMeasureParams {
    PrusaAccelerometer &accelerometer;
    StepEventFlag_t axis_flag;
    bool klipper_mode;
    float acceleration;
    uint32_t cycles;

    /// Configured in setup()
    float accelerometer_sample_period = NAN;

    /// Configured automatically in setup()
    float step_len = NAN;

    /// \returns false on failure
    bool setup(const MicrostepRestorer &microstep_restorer, bool calibrate_accelerometer);
};

struct VibrateMeasureRange {
    float start_frequency;
    float end_frequency;
    float frequency_increment;
};

FrequencyGain3dError vibrate_measure(const VibrateMeasureParams &args, float frequency);

/// \returns false if the measurement should be aborted
using FindBestShaperProgressHook = stdext::inplace_function<bool(input_shaper::Type checked_type, float progress)>;

input_shaper::AxisConfig find_best_shaper(const FindBestShaperProgressHook &progress_hook, const Spectrum &psd, input_shaper::AxisConfig default_config);
