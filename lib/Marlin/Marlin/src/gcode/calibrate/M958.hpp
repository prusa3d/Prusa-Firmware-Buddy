#pragma once

#include <cstddef>
#include <limits>
#include <option/has_local_accelerometer.h>
#include <option/has_remote_accelerometer.h>
#include "Marlin/src/core/types.h"
#include "Marlin/src/module/stepper/trinamic.h"
#include "Marlin/src/module/prusa/accelerometer.h"
#include "Marlin/src/feature/input_shaper/input_shaper_config.hpp"

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
public:
    using State = std::array<uint16_t, 3>;

private:
    State m_mres;

public:
    MicrostepRestorer() {
        save_state(m_mres);
    }

    static void save_state(State &state) {
        LOOP_XYZ(i) {
            state[i] = stepper_microsteps((AxisEnum)i);
        }
    }

    ~MicrostepRestorer() {
        restore_state(m_mres);
    }
    static void restore_state(State &state) {
        while (has_steps()) {
            idle(true, true);
        }
        LOOP_XYZ(i) {
            stepper_microsteps((AxisEnum)i, state[i]);
        }
    }

    const uint16_t *saved_mres() const { return m_mres.data(); }

private:
    static bool has_steps() {
        CRITICAL_SECTION_START;
        bool retval = PreciseStepping::has_step_events_queued();
        CRITICAL_SECTION_END;
        return retval;
    }
};

// Return value for progress hooks
enum class [[nodiscard]] ProgressResult {
    abort,
    progress,
};

// Interface for hooking into get_accelerometer_sample_period()
class AccelerometerProgressHook {
public:
    virtual ~AccelerometerProgressHook() = default;

    // Periodically called by get_accelerometer_sample_period() as it makes progress
    virtual ProgressResult operator()(float progress_ratio) = 0;
};
float get_accelerometer_sample_period(AccelerometerProgressHook &, PrusaAccelerometer &accelerometer);

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

// Interface for hooking into find_best_shaper()
class FindBestShaperProgressHook {
public:
    virtual ~FindBestShaperProgressHook() = default;

    // Periodically called by find_best_shaper() as it makes progress
    virtual ProgressResult operator()(input_shaper::Type, float progress_ratio) = 0;
};
input_shaper::AxisConfig find_best_shaper(FindBestShaperProgressHook &, const Spectrum &psd, input_shaper::AxisConfig default_config);
