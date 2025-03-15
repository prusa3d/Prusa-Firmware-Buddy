#pragma once

#include "common.hpp"

#include <cassert>
#include <functional>
#include <tuple>
#include <optional>
#include <vector>
#include <atomic>
#include <expected>

#include <module/prusa/accelerometer.h>
#include <core/types.h>

namespace phase_stepping {

/**
 * Structure representing a metadata of motor parameter estimation measurement.
 */
struct SamplesAnnotation {
    float sampling_freq;
    bool movement_ok;
    PrusaAccelerometer::Error accel_error;

    // Planned times of the features in the recorded samples
    float start_marker;
    float end_marker;
    float signal_start;
    float signal_end;

    bool is_valid() const {
        return movement_ok && accel_error == PrusaAccelerometer::Error::none && sampling_freq > 0;
    }
};

/**
 * Structure representing a sweep of motor parameters.
 */
struct SweepParams {
    float pha_start;
    float pha_end;
    float mag_start;
    float mag_end;

    SweepParams reverse() const {
        return { pha_end, pha_start, mag_end, mag_start };
    }
};

/**
 * Assuming phase stepping is enabled, make a movement during which a parameter
 * sweep is performed. Yield a projection of captured sample to the active axis.
 * Returns accelerometer sampling frequency, or 0 on error.
 */
SamplesAnnotation capture_param_sweep_samples(AxisEnum axis, float speed, float revs,
    int harmonic, const SweepParams &params, const std::function<void(float)> &yield_sample);

/**
 * Make an accelerated movement and capture samples. Return accelerometer
 * sampling frequency, or 0 on error.
 */
SamplesAnnotation capture_speed_sweep_samples(AxisEnum axis, float start_speed, float end_speed,
    float revs, const std::function<void(float)> &yield_sample);

/**
 * Calibration routine notifies about the progress made via this class. Subclass
 * it and pass it to the calibration routine.
 */
class CalibrateAxisHooks {
public:
    virtual ~CalibrateAxisHooks() = default;

    /**
     * Report initial characterization started
     */
    virtual void on_motor_characterization_start() = 0;

    /**
     * Report initial characterization is done. Phases is the number of
     * calibration phases that will follow
     */
    virtual void on_motor_characterization_result(int phases) = 0;

    /**
     * Report beginning of the new phase
     */
    virtual void on_enter_calibration_phase(int phase) = 0;

    /**
     * Report result in percents for given calibration phase. Lower = better.
     */
    virtual void on_calibration_phase_result(float forward_score, float backward_score) = 0;

    /**
     * Report calibration termination
     */
    virtual void on_termination() = 0;

    enum class ContinueOrAbort {
        Continue,
        Abort,
    };
    virtual ContinueOrAbort on_idle() = 0;
};

/**
 * Assuming the printer is homed, calibrate given axis. The progress is reported
 * via hooks. The routine is blocking.
 *
 * Returns an array with forward and backward calibration
 */
std::expected<std::array<MotorPhaseCorrection, 2>, const char *>
calibrate_axis(AxisEnum axis, CalibrateAxisHooks &hooks);

/**
 * Reset runtime current lookup tables for axis.
 */
void reset_compensation(AxisEnum axis);

} // namespace phase_stepping
