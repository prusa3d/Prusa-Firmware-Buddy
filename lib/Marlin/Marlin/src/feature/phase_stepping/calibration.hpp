#pragma once

#include <option/has_phase_stepping_calibration.h>
#if !HAS_PHASE_STEPPING_CALIBRATION()
    #error "calibration disabled by configuration"
#endif

#include "common.hpp"

#include <inplace_function.hpp>
#include <tuple>
#include <optional>
#include <vector>
#include <atomic>
#include <expected>

#include <module/prusa/accelerometer.h>
#include <core/types.h>
#include <accelerometer/common_structs.hpp>

namespace phase_stepping {

/**
 * Structure representing a metadata of motor parameter estimation measurement.
 */
struct SamplesAnnotation {
    float sampling_freq;
    bool movement_ok;
    accelerometer::Error accel_error;

    // Planned times of the features in the recorded samples
    float start_marker;
    float end_marker;
    float signal_start;
    float signal_end;

    bool is_valid() const {
        return movement_ok && accel_error == accelerometer::Error::none && sampling_freq > 0;
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

/// Prevent implicit conversions from int16_t to float
struct AccelerometerSample {
    int16_t value;
};

using YieldSample = stdext::inplace_function<void(AccelerometerSample)>;

/**
 * Assuming phase stepping is enabled, make a movement during which a parameter
 * sweep is performed. Yield a projection of captured sample to the active axis.
 * Returns accelerometer sampling frequency, or 0 on error.
 */
SamplesAnnotation capture_param_sweep_samples(AxisEnum axis, float speed, float revs,
    int harmonic, const SweepParams &params, const YieldSample &yield_sample);

/**
 * Make an accelerated movement and capture samples. Return accelerometer
 * sampling frequency, or 0 on error.
 */
SamplesAnnotation capture_speed_sweep_samples(AxisEnum axis, float start_speed, float end_speed,
    float revs, const YieldSample &yield_sample);

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

enum class CalibrateAxisError : uint8_t {
    aborted,
    speed_sweep_movement_failed,
    param_sweep_movement_failed,
    no_peaks_found,
    cannot_find_peaks_in_phase_sweep,
    magnitude_out_of_bounds,
};

const char *to_string(CalibrateAxisError);

using CalibrateAxisResult = std::expected<std::array<MotorPhaseCorrection, 2>, CalibrateAxisError>;

/**
 * Assuming the printer is homed, calibrate given axis. The progress is reported
 * via hooks. The routine is blocking.
 *
 * WARNING! G28_no_parser(true, true, false, { .only_if_needed = true, .z_raise = 3, .precise = false });
 * should be executed before calling this function.
 * Within this function it may be called as well and then it needs lot of stack space.
 *
 * Returns an array with forward and backward calibration
 * The result is passed by reference, because it's quite big (several > 100 B)
 */
void calibrate_axis(AxisEnum axis, CalibrateAxisHooks &hooks, CalibrateAxisResult &result);

} // namespace phase_stepping
