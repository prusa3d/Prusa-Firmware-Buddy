/*
 * CoreXY precise homing refinement - implementation
 * TODO: @wavexx: Add some documentation
 */

#include "homing_corexy.hpp"

#include "corexy_transform.hpp"

// sanity checks
#include <option/has_crash_detection.h>
#include <option/has_precise_homing.h>
#if HAS_PRECISE_HOMING()
    #error "HAS_PRECISE_HOMING_COREXY() is mutually exclusive with HAS_PRECISE_HOMING()"
#endif
#ifdef HAS_TMC_WAVETABLE
    // #error dead code found by automatic analyses (see BFW-5461)
    // Wavetable restoration needs to happen after refinement succeeds, and
    // not per-axis as currently done. Ensure the setting is not enabled by mistake.
    #error "HAS_PRECISE_HOMING_COREXY() is not compatible with HAS_TMC_WAVETABLE"
#endif

#include "../planner.h"
#include "../stepper.h"
#include "../endstops.h"

#include <feature/motordriver_util.h>
#include <lcd/ultralcd.h>
#include <configuration.hpp> // for axis_home_*_diff

#if HAS_CRASH_DETECTION()
    #include "feature/prusa/crash_recovery.hpp"
#endif

#include <bsod.h>
#include <raii/scope_guard.hpp>
#include <feature/phase_stepping/phase_stepping.hpp>
#include <feature/input_shaper/input_shaper_config.hpp>
#include <config_store/store_instance.hpp>
#include <metric.h>
#include <cmath_ext.h>
#include <option/has_toolchanger.h>

#include <utility>
using std::make_pair;
using std::pair;

#if HAS_TRINAMIC && defined(XY_HOMING_MEASURE_SENS_MIN)
    #include <configuration.hpp>
#endif

#pragma GCC diagnostic warning "-Wdouble-promotion"

// Outward measurement flex allowance (in phase cycles) for long, flexing gantries. Defined per
// printer in Configuration_<printer>_adv.h; rigid frames leave it at 0.
#ifndef XY_HOMING_GANTRY_FLEX_CYCLES
    #define XY_HOMING_GANTRY_FLEX_CYCLES 0
#endif

namespace {
// AB phase grid type for type checking
struct PhaseGridTag {};
using ab_grid_t = XYval<int32_t, PhaseGridTag>;

// Electrical phase alignment position: set to 45' for maximum two-phase current holding torque
static constexpr int16_t zero_phase_mscnt_angle = 1024 / 8;

// Number of measurement probes to average
static constexpr int16_t measure_probe_n = 2;

// Scramble the calibration probing sequence to improve belt redistribution when estimating the
// centroid: units are full AB cycles away from homing corner as given to plan_corexy_abgrid_move()
static constexpr ab_grid_t measure_point_sequence[] = {
    { 1, -3 },
    { -3, -1 },
    { 3, 1 },
    { -1, 3 },
    { 0, 0 },
};

// Instability phase threshold: total width of the invalid period close to the roundoff threshold
// when calculating the resulting grid value.
static constexpr float unstable_phase_threshold = 1. / 4;

// Origin estimate sweep resolution: grid-offset samples per axis
static constexpr int origin_sweep_res = 128;

// Per-point minimum 95% cycle measurement convergence and maximum pass count
static constexpr float origin_min_cycle_ci = 0.5f; // half a cycle width
static constexpr uint16_t origin_max_passes = 10;
static_assert(origin_max_passes >= 2, "CI convergence needs at least 2 samples per point");

namespace internal {
    bool home_unstable = false; ///< Last homing stability state
    uint8_t probe_id = 0; ///< Probe id for metric cross-referencing
    uint8_t refine_id = 0; ///< Refine count for metric cross-referencing
    uint8_t cal_id = 0; ///< Calibration count for metric cross-referencing
} // namespace internal
} // namespace

METRIC_DEF(metric_phxy_meas, "phxy_meas", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_phxy_probe, "phxy_probe", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_phxy_sens, "phxy_sens", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_phxy_home, "phxy_home", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
METRIC_DEF(metric_phxy_orig, "phxy_orig", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);

/// Convert raw AB steps to XY mm and position in mini-steps
static void corexy_ab_to_xy(const ab_steps_t &steps, MachinePosXY &mm, xy_msteps_t &pos_msteps) {
    const float x = static_cast<float>(steps.a + steps.b) / 2.f;
    const float y = static_cast<float>(CORESIGN(steps.a - steps.b)) / 2.f;
    mm.x = x * planner.mm_per_step[X_AXIS];
    mm.y = y * planner.mm_per_step[Y_AXIS];
    pos_msteps.x = LROUND(x * PLANNER_STEPS_MULTIPLIER);
    pos_msteps.y = LROUND(y * PLANNER_STEPS_MULTIPLIER);
}

/// Convert raw AB steps to XY mm and position in mini-steps, filling others from current state
static void corexy_ab_to_xyze(const ab_steps_t &steps, MachinePosXYZE &mm, xyze_msteps_t &pos_msteps) {
    pos_msteps = planner.get_position_msteps();
    {
        MachinePosXY xy;
        xy_msteps_t xy_msteps;
        corexy_ab_to_xy(steps, xy, xy_msteps);
        mm.set(xy);
        pos_msteps.set(xy_msteps);
    }
    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
        mm[i] = planner.get_axis_position_mm((AxisEnum)i);
    }
}

static void plan_raw_move(const MachinePosXYZE target_mm, const xyze_msteps_t target_pos, const feedRate_t fr_mm_s) {
    planner._buffer_msteps(target_pos, target_mm, fr_mm_s, PhysicalToolIndex::currently_selected(), { .raw_block = true });
    planner.synchronize();
}

static void plan_corexy_raw_move(const ab_steps_t &target_steps_ab, const feedRate_t fr_mm_s) {
    // reconstruct full final position
    MachinePosXYZE target_mm;
    xyze_msteps_t target_pos_msteps;
    corexy_ab_to_xyze(target_steps_ab, target_mm, target_pos_msteps);

    plan_raw_move(target_mm, target_pos_msteps, fr_mm_s);
}

/// TMC µsteps(phase) per Marlin µsteps
static constexpr int16_t phase_per_ustep(const AxisEnum axis) {
    // Originally, we read the microstep configuration from the driver; this no
    // longer make sense with 256 microsteps.
    // Thus, we use the printer defaults instead of stepper_axis(axis).microsteps();
    debug_assert(axis <= AxisEnum::Z_AXIS);
    static const int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };
    return 256 / MICROSTEPS[axis];
};

/// TMC full cycle µsteps per Marlin µsteps
static constexpr int16_t phase_cycle_steps(const AxisEnum axis) {
    return 1024 / phase_per_ustep(axis);
}

static int16_t axis_mscnt(const AxisEnum axis) {
#if HAS_PHASE_STEPPING()
    return phase_stepping::logical_ustep(axis);
#else
    // #error dead code found by automatic analyses (see BFW-5461)
    return stepper_axis(axis).MSCNT();
#endif
}

static int16_t phase_backoff_steps(const AxisEnum axis) {
    int16_t effectorBackoutDir; // Direction in which the effector mm coordinates move away from endstop.
    int16_t stepperCountDir; // Direction in which the TMC µstep count(phase) increases.
    switch (axis) {
    case X_AXIS:
        effectorBackoutDir = -X_HOME_DIR;
        stepperCountDir = INVERT_X_DIR ? -1 : 1;
        break;
    case Y_AXIS:
        effectorBackoutDir = -Y_HOME_DIR;
        stepperCountDir = INVERT_Y_DIR ? -1 : 1;
        break;
    default:
        bsod("invalid backoff axis");
    }

    const int16_t phaseCurrent = axis_mscnt(axis); // The TMC µsteps(phase) count of the current position
    const int16_t phaseZero = (phaseCurrent + zero_phase_mscnt_angle) % 1024; // zero phase
    const int16_t phaseDelta = ((stepperCountDir < 0) == (effectorBackoutDir < 0) ? phaseZero : 1024 - phaseZero);
    const int16_t phasePerStep = phase_per_ustep(axis);
    return int16_t((phaseDelta + phasePerStep / 2) / phasePerStep) * effectorBackoutDir;
}

static bool phase_aligned(AxisEnum axis) {
    const int16_t phase_cur = axis_mscnt(axis);
    const int16_t phase_zero = (phase_cur + zero_phase_mscnt_angle) % 1024;
    const int16_t ustep_max = phase_per_ustep(axis) / 2;
    return (phase_zero <= ustep_max || phase_zero >= (1024 - ustep_max));
}

/**
 * @brief Helper class to adjust machine settings for AB measurements using measure_axis_distance().
 *
 * Only the non-measured axis stepper is adjusted: the measured stepper is setup within
 * measure_axis_distance() itself.
 */
class MeasurementGuard {
    static inline unsigned nesting = 0; // helper to ensure machine settings are set exactly once

    // "other" stepper original settings
    decltype(stepperX) &other_stepper;
    int32_t other_orig_cur;
    float other_orig_hold;

    // original IS settings
    std::optional<input_shaper::AxisConfig> is_config_orig[2];

public:
    [[nodiscard]] MeasurementGuard(AxisEnum other_axis)
        : other_stepper(stepper_axis(other_axis)) {
        // check for, but disallow nesting
        ++nesting;
        debug_assert(nesting == 1);

        planner.synchronize();
        other_orig_cur = other_stepper.rms_current();
        other_orig_hold = other_stepper.hold_multiplier();
#ifdef XY_HOMING_HOLDING_CURRENT
        other_stepper.rms_current(XY_HOMING_HOLDING_CURRENT, 1.f);
#endif

        is_config_orig[A_AXIS] = input_shaper::get_axis_config(A_AXIS);
        is_config_orig[B_AXIS] = input_shaper::get_axis_config(B_AXIS);
        input_shaper::set_axis_config(A_AXIS, std::nullopt);
        input_shaper::set_axis_config(B_AXIS, std::nullopt);
    }

    ~MeasurementGuard() {
        --nesting;
        planner.synchronize();
        other_stepper.rms_current(other_orig_cur, other_orig_hold);
        input_shaper::set_axis_config(A_AXIS, is_config_orig[A_AXIS]);
        input_shaper::set_axis_config(B_AXIS, is_config_orig[B_AXIS]);
    }

    static bool is_active() {
        return nesting > 0;
    }
};

/// Axis measurement settings
struct measure_axis_params {
    float feedrate;
    uint16_t current;
#if HAS_TRINAMIC
    int8_t sensitivity;
#endif
};

/**
 * @brief Measure axis distance precisely
 * @param axis Axis to measure
 * @param origin_steps Initial stepper position
 * @param dist Travel step direction/distance
 * @param m_steps Measured steps at the end of the travel
 * @param m_dist Measured distance at the end of the travel
 * @param fr_mm_s Service move feedrate
 * @param params Measured axis/stepper parameters
 * @return Endstop hit state (true when hit)
 */
static bool measure_axis_distance(const AxisEnum axis, const ab_steps_t origin_steps, const int32_t dist,
    int32_t &m_steps, float &m_dist, const float fr_mm_s, const measure_axis_params &params) {
    const AxisEnum fixed_axis = (axis == B_AXIS ? A_AXIS : B_AXIS);

    // full initial position
    const abce_steps_t initial_steps { origin_steps.a, origin_steps.b, stepper.position(C_AXIS), stepper.position(E_AXIS) };
    MachinePosXYZE initial_mm;
    corexy_ab_to_xyze(initial_steps.xy(), initial_mm);

    // full target position
    abce_steps_t target_steps = initial_steps;
    target_steps[axis] += dist;

    MachinePosXYZE target_mm;
    target_mm.set(corexy_ab_to_xy(target_steps.xy()));

    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
        target_mm[i] = initial_mm[i];
    }

    // ensure the fixed axis doesn't move due to CORE AB->XY->AB rounding: recalculate msteps
    // by difference directly since this is how the raw move plans the movement
    const xyze_msteps_t initial_pos_msteps = planner.get_position_msteps();
    xyze_msteps_t target_pos_msteps = initial_pos_msteps;
    const int32_t dist_msteps = dist * PLANNER_STEPS_MULTIPLIER;
    if (axis == B_AXIS) {
        target_pos_msteps[X_AXIS] += dist_msteps / 2;
        target_pos_msteps[Y_AXIS] -= dist_msteps / 2;
    } else {
        target_pos_msteps[X_AXIS] += dist_msteps / 2;
        target_pos_msteps[Y_AXIS] += dist_msteps / 2;
    }

    // prepare stepper for the move
    debug_assert(MeasurementGuard::is_active());
    const sensorless_t stealth_states = start_sensorless_homing_per_axis(axis);
    auto &axis_stepper = stepper_axis(axis);
    const int32_t axis_orig_cur = axis_stepper.rms_current();
    const float axis_orig_hold = axis_stepper.hold_multiplier();
    axis_stepper.rms_current(params.current, 1.f);
#if HAS_TRINAMIC
    const int8_t axis_orig_sens = axis_stepper.sgt();
    axis_stepper.sgt(params.sensitivity);
#endif
    ScopeGuard state_restorer([&]() {
#if HAS_TRINAMIC
        axis_stepper.sgt(axis_orig_sens);
#endif
        axis_stepper.rms_current(axis_orig_cur, axis_orig_hold);
    });

    // move towards the endstop
    endstops.enable(true);
    plan_raw_move(target_mm, target_pos_msteps, params.feedrate);
    const uint8_t hit = endstops.trigger_state();
    endstops.not_homing();

    abce_steps_t hit_steps;
    MachinePosXYZE hit_mm;
    if (hit) {
        // resync position from steppers to get hit position
        endstops.hit_on_purpose();
        planner.reset_position();
        hit_steps = { stepper.position(A_AXIS), stepper.position(B_AXIS), stepper.position(C_AXIS), stepper.position(E_AXIS) };
        corexy_ab_to_xyze(hit_steps.xy(), hit_mm);
    } else {
        hit_steps = target_steps;
        hit_mm = target_mm;
    }
    end_sensorless_homing_per_axis(axis, stealth_states);

    // move back to starting point
    plan_raw_move(initial_mm, initial_pos_msteps, fr_mm_s);
    if (planner.draining()) {
        return false;
    }

    // sanity checks
    if (hit_steps[fixed_axis] != initial_steps[fixed_axis] || initial_steps[fixed_axis] != stepper.position(fixed_axis)) {
        bsod("fixed axis moved unexpectedly");
    }
    if (initial_steps[axis] != stepper.position(axis)) {
        bsod("measured axis didn't return");
    }

    // result values
    m_steps = hit_steps[axis] - initial_steps[axis];
    m_dist = hypotf(hit_mm[X_AXIS] - initial_mm[X_AXIS], hit_mm[Y_AXIS] - initial_mm[Y_AXIS]);

    metric_record_custom(&metric_phxy_meas, ",a=%u,dir=%i p=%u,s=%li,d=%.3f",
        axis, dist >= 0 ? 1 : -1, internal::probe_id, (long)m_steps, (double)m_dist);

    return hit;
}

/// Return measure axis default parameters
static measure_axis_params measure_axis_defaults(const AxisEnum axis) {
    measure_axis_params params;

#if HAS_TRINAMIC
    #ifdef XY_HOMING_MEASURE_SENS
    // #error dead code found by automatic analyses (see BFW-5461)
    params.sensitivity = XY_HOMING_MEASURE_SENS;
    #else
    params.sensitivity = (axis == A_AXIS ? X_STALL_SENSITIVITY : Y_STALL_SENSITIVITY);
    #endif
#endif
#ifdef XY_HOMING_MEASURE_FR
    params.feedrate = XY_HOMING_MEASURE_FR;
#else
    params.feedrate = homing_feedrate(axis);
#endif
#ifdef XY_HOMING_MEASURE_CURRENT
    params.current = XY_HOMING_MEASURE_CURRENT;
#else
    params.current = (axis == A_AXIS ? X_CURRENT_HOME : Y_CURRENT_HOME);
#endif

    return params;
}

/**
 * @brief Call measure_axis_distance() with calibrated parameters
 * @see measure_axis_distance() for parameter documentation
 **/
static bool measure_axis_distance(const AxisEnum axis, const ab_steps_t origin_steps, const int32_t dist,
    int32_t &m_steps, float &m_dist, const float fr_mm_s) {
    measure_axis_params params;

#if HAS_TRINAMIC && defined(XY_HOMING_MEASURE_SENS_MIN)
    // get paramers from calibration
    const CoreXYHomeTMCSens calibrated_sens = config_store().corexy_home_tmc_sens.get();
    if (calibrated_sens.uninitialized()) {
        bsod("axis measurement without calibration");
    }

    params.sensitivity = calibrated_sens.sensitivity;
    params.feedrate = calibrated_sens.feedrate;
    params.current = calibrated_sens.current;
#else
    // static defaults
    params = measure_axis_defaults(axis);
#endif

    return measure_axis_distance(axis, origin_steps, dist, m_steps, m_dist, fr_mm_s, params);
}

/**
 * @brief Sum "axis" along the XY*val<> sequence "seq"
 * @tparam T Sequence type (normally deduced)
 * @tparam S sum type (normally deduced)
 * @param seq Sequence of XY*val
 * @param size Size of the sequence
 * @param axis Axis to sum
 * @return Resulting sum
 */
template <typename T, typename S = decltype(T::x)>
S sum_along(const T *seq, const size_t size, const size_t axis) {
    S sum = 0;
    for (size_t i = 0; i != size; ++i) {
        sum += seq[i][axis];
    }
    return sum;
}

/**
 * @brief Distance required to reach the target feedrate
 * @param fr_mm_s Target feedrate
 * @return Distance to cruising speed from zero velocity
 * Calculation performed with current planner travel settings
 */
static float travel_accel_distance(const float fr_mm_s) {
    return SQR(fr_mm_s) / (2.f * planner.settings.travel_acceleration);
}

/// Maximum allowed homing distance difference per axis (max - min)
static float axis_home_max_abs_diff(const AxisEnum axis) {
    return axis_home_max_diff(axis) - axis_home_min_diff(axis);
}

/**
 * @brief Part of precise homing.
 * @param axis Physical axis to measure
 * @param ab_off Expected relative AB cycle offset from origin
 * @param c_dist AB cycle distance from the endstop
 * @param fr_mm_s Service move feedrate
 * @return True on success
 */
static bool measure_phase_cycles(const AxisEnum axis, const ab_grid_t &ab_off, xy_pos_t &c_dist, const float fr_mm_s) {
    // prepare for repeated measurements
    const AxisEnum other_axis = (axis == B_AXIS ? A_AXIS : B_AXIS);
    MeasurementGuard setup_guard(other_axis);
    ++internal::probe_id;

    // allow half a cycle of measurement tolerance, validation will further restrict allowance to 1/4
    const int32_t measure_bump_max_err_steps = phase_cycle_steps(axis) / 2;
    const float measure_bump_max_err_mm = planner.mm_per_step[axis] * measure_bump_max_err_steps;

    const int32_t measure_dir = (axis == B_AXIS ? -X_HOME_DIR : -Y_HOME_DIR);
    const ab_steps_t origin_steps = { stepper.position(A_AXIS), stepper.position(B_AXIS) };

    // expected exact corner distances given current offset
    const int32_t exp_d = static_cast<int32_t>(XY_HOMING_ORIGIN_OFFSET * 2 / planner.mm_per_step[axis]
        + ab_off[0] * phase_cycle_steps(other_axis));
    const int32_t exp_a = ab_off[1] * phase_cycle_steps(axis);
    const int32_t exp_dist_steps[2] = { exp_d + exp_a, exp_d - exp_a };

    // absolute tolerance for the travel move:
    // - maximum diagonal shift of classic homing (maximum relative difference or absolute bump tolerance)
    // - 1*cycle due to the maximum outwards A+B phase alignment
    // - bump tolerance allowed by the new measurement
    const float home_max_diff_mm = max(max(axis_home_max_abs_diff(A_AXIS), axis_home_max_abs_diff(B_AXIS))
            * std::numbers::sqrt2_v<float>,
        measure_bump_max_err_mm);
    const int32_t measure_eps_steps = static_cast<int32_t>(home_max_diff_mm / planner.mm_per_step[axis]
        + phase_cycle_steps(axis) + measure_bump_max_err_steps);
    // Long gantries (e.g. XL) flex away from the endstop when the carriage is pushed out before
    // measuring, lengthening the measured travel by a small, roughly constant amount. Absorb this
    // in the late-trigger (max) bound of both directions, keeping the early-trigger (min) bound
    // tight. 0 on rigid frames.
    const int32_t measure_eps_steps_max = measure_eps_steps
        + static_cast<int32_t>(XY_HOMING_GANTRY_FLEX_CYCLES * phase_cycle_steps(axis));
    const int32_t measure_acc_steps = static_cast<int32_t>(travel_accel_distance(fr_mm_s)
        * std::numbers::sqrt2_v<float> / planner.mm_per_step[axis]);

    // keep the average of at least n values having less than max_err of separation between each
    ab_steps_t p_steps[measure_probe_n];
    xy_pos_t p_dist[measure_probe_n];

    // keep sampling *while* cycling on retries (we don't know which probes are good yet)
    uint8_t retries = 0;
    for (uint8_t idx = 0; retries <= XY_HOMING_ORIGIN_BUMP_RETRIES;) {
        const uint8_t slot = idx % measure_probe_n;

        // measure distance B-/B+
        for (uint8_t dir = 0; dir != 2;) {
            const int32_t dist_steps = (exp_dist_steps[dir] + measure_eps_steps_max + measure_acc_steps) * (dir ? measure_dir : -measure_dir);
            const bool hit = measure_axis_distance(axis, origin_steps, dist_steps, p_steps[slot][dir], p_dist[slot][dir], fr_mm_s);
            const int32_t exp_dir_steps_min = exp_dist_steps[dir] - measure_eps_steps;
            const int32_t exp_dir_steps_max = exp_dist_steps[dir] + measure_eps_steps_max;

            // record all probe metric data, split due to maximum size requirements
            const uint32_t ts = ticks_us();
            metric_record_custom_at_time(&metric_phxy_probe, ts, ",ax=%u,a=%ld,b=%ld p=%u,r=%u,h=%d",
                axis, static_cast<long>(ab_off[0]), static_cast<long>(ab_off[1]),
                internal::probe_id, idx, hit);
            metric_record_custom_at_time(&metric_phxy_probe, ts, ",ax=%u,a=%ld,b=%ld min=%ld,max=%ld,dist=%ld",
                axis, static_cast<long>(ab_off[0]), static_cast<long>(ab_off[1]),
                static_cast<long>(exp_dir_steps_min), static_cast<long>(exp_dir_steps_max), static_cast<long>(p_steps[slot][dir]));

            if (!hit) {
                // we can't possibly reach the endstop by retrying, abort
                SERIAL_ECHOLNPAIR("endstop ", (dir == 0 ? '-' : '+'), ": not reached");
                ui.status_printf_P(0, "Endstop not reached");
                return false;
            }

            // keep signs positive
            p_steps[slot][dir] = abs(p_steps[slot][dir]);
            p_dist[slot][dir] = abs(p_dist[slot][dir]);

            if (p_steps[slot][dir] > exp_dir_steps_max) {
                // calculated travel ends within deceleration, wrong position or short travel
                SERIAL_ECHOLNPAIR("endstop ", (dir == 0 ? '-' : '+'), ": planned travel too short ",
                    p_steps[slot][dir], ">", exp_dir_steps_max);
                ui.status_printf_P(0, "Endstop not reached");
                return false;
            }

            if (p_steps[slot][dir] < exp_dir_steps_min) {
                // early trigger, retry the probe in the same direction
                SERIAL_ECHOLNPAIR("endstop ", (dir == 0 ? '-' : '+'), ": early trigger ",
                    p_steps[slot][dir], "<", exp_dir_steps_min);
                ui.status_printf_P(0, "Endstop early trigger");
                if (++retries <= XY_HOMING_ORIGIN_BUMP_RETRIES) {
                    continue;
                }
            }

            ++dir;
        }
        if (retries > XY_HOMING_ORIGIN_BUMP_RETRIES) {
            break;
        }

        // check for maximum probe difference in the window
        float p_diff[2] = { 0, 0 };
        if (idx >= measure_probe_n && measure_probe_n > 1) {
            for (uint8_t n = 0; n < measure_probe_n - 1; ++n) {
                LOOP_XY(i) {
                    p_diff[i] = max(p_diff[i], abs(p_dist[n][i] - p_dist[n + 1][i]));
                }
            }
        }

        if (idx >= measure_probe_n) {
            if (p_diff[0] < measure_bump_max_err_mm && p_diff[1] < measure_bump_max_err_mm) {
                break;
            }
            ++retries;
        }
        ++idx;
    }
    if (retries > XY_HOMING_ORIGIN_BUMP_RETRIES) {
        SERIAL_ECHOLN("axis measurement failed: too many failed probes");
        ui.status_printf_P(0, "Axis measurement failed");
        return false;
    }

    // calculate the absolute cycle coordinates
    const float d1 = sum_along(p_steps, measure_probe_n, 0) / float(measure_probe_n);
    const float d2 = sum_along(p_steps, measure_probe_n, 1) / float(measure_probe_n);
    const float d = d1 + d2;
    const float a = d / 2.f;
    const float b = d1 - a;

    c_dist[0] = a / float(phase_cycle_steps(other_axis));
    c_dist[1] = b / float(phase_cycle_steps(axis));

    if (DEBUGGING(LEVELING)) {
        // measured distance and cycle
        const xy_pos_t m_dist = {
            sum_along(p_dist, measure_probe_n, 0) / float(measure_probe_n),
            sum_along(p_dist, measure_probe_n, 1) / float(measure_probe_n),
        };

        SERIAL_ECHOLNPAIR("home ", physical_axis_codes[axis], "+ steps 0:", p_steps[0][1], " 1:", p_steps[1][1],
            " cycle A:", c_dist[0], " mm:", m_dist[1]);
        SERIAL_ECHOLNPAIR("home ", physical_axis_codes[axis], "- steps 0:", p_steps[0][0], " 1:", p_steps[1][0],
            " cycle B:", c_dist[1], " mm:", m_dist[0]);
    }
    return true;
}

/// Return true if the point is too close to the phase grid halfway point
static bool point_is_unstable(const xy_pos_t &c_dist, const xy_pos_t &origin) {
    LOOP_XY(axis) {
        // The threshold is a total width, but we're comparing against one-sided distances here
        if (abs(fmod(abs(c_dist[axis] - origin[axis]), 1.f) - 0.5f) < unstable_phase_threshold / 2) {
            return true;
        }
    }
    return false;
}

/// Translate fractional cycle distance by origin and round to final AB grid
static ab_grid_t cdist_translate(const xy_pos_t &c_dist, const xy_pos_t &origin) {
    ab_grid_t c_ab;
    LOOP_XY(axis) {
        int32_t o_int = int32_t(roundf(origin[axis]));
        c_ab[axis] = int32_t(roundf(c_dist[axis] - origin[axis])) + o_int;
    }
    return c_ab;
}

/// Convert an absolute AB cycle origin to the diagonal mm distance from the endstop
static xy_pos_t origin_to_distance(const AxisEnum axis, const xy_pos_t &origin) {
    const AxisEnum other_axis = (axis == B_AXIS ? A_AXIS : B_AXIS);
    const float a = origin[A_AXIS] * phase_cycle_steps(other_axis);
    const float b = origin[B_AXIS] * phase_cycle_steps(axis);
    const float step_to_mm = planner.mm_per_step[axis] / std::numbers::sqrt2_v<float>;
    return { (a + b) * step_to_mm, CORESIGN(a - b) * step_to_mm };
}

/**
 * @brief plan a relative move by full AB cycles around origin_steps
 * @param ab_off full AB cycles away from homing corner
 * @return new step position
 */
static ab_steps_t plan_corexy_abgrid_move(const ab_steps_t &origin_steps, const ab_grid_t &ab_off, const float fr_mm_s) {
    const int32_t a = ab_off[X_HOME_DIR == Y_HOME_DIR ? A_AXIS : B_AXIS] * -Y_HOME_DIR;
    const int32_t b = ab_off[X_HOME_DIR == Y_HOME_DIR ? B_AXIS : A_AXIS] * -X_HOME_DIR;

    ab_steps_t point_steps = {
        origin_steps[A_AXIS] + phase_cycle_steps(A_AXIS) * a,
        origin_steps[B_AXIS] + phase_cycle_steps(B_AXIS) * b
    };

    plan_corexy_raw_move(point_steps, fr_mm_s);
    return point_steps;
}

// Per-grid-point measurement
struct measure_data {
    xy_pos_t c_dist; ///< absolute AB cycle coordinate of the probed cell
};

/**
 * @brief Per-axis mean cycle coordinate along all passes of a measurement point
 * @tparam N array stride
 * @param points in the format measure_data[passes * N + idx]
 * @param passes pass count
 * @param idx point index
 * @return cycle mean
 **/
template <size_t N>
static xy_pos_t point_cycle_mean(const measure_data *points, size_t passes, size_t idx) {
    xy_pos_t mean = { 0.f, 0.f };
    LOOP_XY(axis) {
        float sum = 0.f;
        for (size_t p = 0; p != passes; ++p) {
            sum += points[p * N + idx].c_dist[axis];
        }
        mean[axis] = sum / float(passes);
    }
    return mean;
}

/**
 * @brief Return the maximum absolute range along all passes of a measurement point
 * @tparam N array stride
 * @param points in the format measure_data[passes * N + idx]
 * @param passes pass count
 * @param idx point index
 * @return per-axis cycle range
 **/
template <size_t N>
static xy_pos_t point_cycle_range(const measure_data *points, size_t passes, size_t idx) {
    xy_pos_t c_min;
    xy_pos_t c_max;
    LOOP_XY(axis) {
        c_min[axis] = c_max[axis] = points[idx].c_dist[axis];
        for (size_t p = 1; p != passes; ++p) {
            const auto &data = points[p * N + idx];
            c_min[axis] = std::min(c_min[axis], data.c_dist[axis]);
            c_max[axis] = std::max(c_max[axis], data.c_dist[axis]);
        }
    }
    return { abs(c_max[A_AXIS] - c_min[A_AXIS]), abs(c_max[B_AXIS] - c_min[B_AXIS]) };
}

/// Two-sided 95% student-t critical value for the given degrees of freedom (dof >= 1)
static constexpr float t95_value(const uint16_t dof) {
    // exact for dof 1..10, coarse steps above, asymptoting to the normal z = 1.96
    static constexpr float t[] = {
        12.706f, 4.303f, 3.182f, 2.776f, 2.571f,
        2.447f, 2.365f, 2.306f, 2.262f, 2.228f
    };
    if (dof < 1) {
        return INFINITY;
    }
    if (dof <= 10) {
        return t[dof - 1];
    }
    if (dof <= 15) {
        return 2.131f;
    }
    if (dof <= 20) {
        return 2.086f;
    }
    if (dof <= 30) {
        return 2.042f;
    }
    return 1.96f;
}

/**
 * @brief Return the cycle width 95% CI of the worst axis of a point
 * @tparam N array stride
 * @param points in the format measure_data[passes * N + idx]
 * @param passes pass count
 * @param idx point index
 * @return CI
 **/
template <size_t N>
static float point_cycle_ci(const measure_data *points, size_t passes, size_t idx) {
    if (passes < 2) {
        return INFINITY;
    }
    const xy_pos_t mean = point_cycle_mean<N>(points, passes, idx);
    float worst = 0.f;
    LOOP_XY(axis) {
        float ss = 0.f;
        for (size_t p = 0; p != passes; ++p) {
            ss += SQR(points[p * N + idx].c_dist[axis] - mean[axis]);
        }
        const float stddev = sqrtf(ss / float(passes - 1));
        const float halfwidth = t95_value(passes - 1) * stddev / sqrtf(float(passes));
        worst = std::max(worst, halfwidth * 2);
    }
    return worst;
}

/**
 * @brief Estimate the grid origin by chamfer distance under translation.
 * @param points measured grid points
 * @param anchor_ab AB offset of points[0]
 * @return absolute origin
 *
 * The fractional cycle coordinate is modular (the phase grid repeats every cycle). Sweep the grid
 * offset at 1/origin_sweep_res and keep the offset minimizing the summed squared 2D distance from
 * each point to its nearest node in order to penalize outliers.
 **/
template <size_t N>
static xy_pos_t estimate_origin(const measure_data (&points)[N], size_t count, const ab_grid_t &anchor_ab) {
    const xy_pos_t origin = {
        points[0].c_dist[A_AXIS] - float(anchor_ab[A_AXIS]),
        points[0].c_dist[B_AXIS] - float(anchor_ab[B_AXIS]),
    };

    // calculate per-point fractional distance
    xy_pos_t points_frac[N];
    for (size_t i = 0; i != count; ++i) {
        points_frac[i][A_AXIS] = points[i].c_dist[A_AXIS] - floorf(points[i].c_dist[A_AXIS]);
        points_frac[i][B_AXIS] = points[i].c_dist[B_AXIS] - floorf(points[i].c_dist[B_AXIS]);
    }

    float best_cost = INFINITY;
    xy_pos_t best_delta = { 0.f, 0.f };
    for (int ia = 0; ia != origin_sweep_res; ++ia) {
        const float da = float(ia) / origin_sweep_res;
        for (int ib = 0; ib != origin_sweep_res; ++ib) {
            const float db = float(ib) / origin_sweep_res;
            float cost = 0.f;
            for (size_t i = 0; i != count; ++i) {
                // squared distance to the nearest node, per axis in range [0, 0.5]
                const float ra = fabsf(points_frac[i][A_AXIS] - da);
                const float rb = fabsf(points_frac[i][B_AXIS] - db);
                cost += SQR(fminf(ra, 1.f - ra)) + SQR(fminf(rb, 1.f - rb));
            }
            if (cost < best_cost) {
                best_cost = cost;
                best_delta[A_AXIS] = da;
                best_delta[B_AXIS] = db;
            }
        }

        // the fit can take a while depending on the pass count
        idle(true);
    }

    return {
        roundf(origin[A_AXIS] - best_delta[A_AXIS]) + best_delta[A_AXIS],
        roundf(origin[B_AXIS] - best_delta[B_AXIS]) + best_delta[B_AXIS],
    };
}

static bool measure_origin_multipoint(AxisEnum axis, const ab_steps_t &origin_steps,
    xy_pos_t &origin, xy_pos_t &distance, const float fr_mm_s) {
    // to conserve stack space, store all measured points in a flat array and recompute derived
    // values on the fly at each pass (those are cheap to compute)
    constexpr size_t N = std::size(measure_point_sequence);
    measure_data measure[N * origin_max_passes];

    // Re-measure the whole grid until every point is known to be within `origin_min_cycle_ci`.
    // Abort immediately during skips and/or if we cannot converge within `origin_max_passes`.
    bool converged = false;
    size_t passes = 0;
    while (!converged && passes < origin_max_passes) {
        // cycle through grid points and measure one sample per point
        for (size_t i = 0; i != N; ++i) {
            const auto &seq = measure_point_sequence[i];
            auto &data = measure[passes * N + i];

            plan_corexy_abgrid_move(origin_steps, seq, fr_mm_s);
            if (planner.draining()) {
                return false;
            }

            if (!measure_phase_cycles(axis, seq, data.c_dist, fr_mm_s)) {
                return false;
            }
            idle(true); // allow some time to flush the metrics buffer
        }
        ++passes;

        if (passes >= 2) {
            // verify if we did converge on all points
            converged = true;
            for (size_t i = 0; i != N; ++i) {
                const auto &seq = measure_point_sequence[i];

                const xy_pos_t c_range = point_cycle_range<N>(measure, passes, i);
                if (c_range[A_AXIS] >= 1 || c_range[B_AXIS] >= 1) {
                    // if the maximum range is greater than a full cycle we likely skipped
                    SERIAL_ECHOLNPAIR("home calibration point (", seq[A_AXIS], ",", seq[B_AXIS],
                        ") range too large A:", c_range[A_AXIS], " B:", c_range[B_AXIS]);
                    return false;
                }

                const float c_ci = point_cycle_ci<N>(measure, passes, i);
                if (c_ci >= origin_min_cycle_ci) {
                    SERIAL_ECHOLNPAIR("home calibration point (", seq[A_AXIS], ",", seq[B_AXIS],
                        ") not converged after ", passes, " passes");
                    converged = false;
                    break;
                }
            }
        }
    }
    if (!converged || !passes) {
        // measurements never stabilized within the target precision: reject calibration
        internal::home_unstable = true;
        SERIAL_ECHOLNPAIR("home grid measurement did not converge");
        return false;
    }

    const size_t count = passes * N;
    origin = estimate_origin(measure, count, measure_point_sequence[0]);

    metric_record_custom(&metric_phxy_orig, ",a=%u,t=\"o\" c=%u,o0=%.3f,o1=%.3f,p=%u",
        axis, internal::cal_id, (double)origin[0], (double)origin[1], passes);

    // verify every individual sample against the computed centroid
    const ab_grid_t o_int = { int32_t(roundf(origin[A_AXIS])), int32_t(roundf(origin[B_AXIS])) };
    for (size_t k = 0; k != count; ++k) {
        const auto &seq = measure_point_sequence[k % N];
        auto &data = measure[k];

        const ab_grid_t c_ab = cdist_translate(data.c_dist, origin);
        const ab_grid_t c_diff = c_ab - seq - o_int;
        const bool c_unstable = point_is_unstable(data.c_dist, origin);
        const bool c_invalid = c_diff[A_AXIS] || c_diff[B_AXIS];

        metric_record_custom(&metric_phxy_orig, ",a=%u,t=\"p\" c=%u,p=%u,c0=%li,c1=%li,s=%u",
            axis, internal::cal_id, k, (long)c_ab[0], (long)c_ab[1], c_unstable);
        metric_record_custom(&metric_phxy_orig, ",a=%u,t=\"p\" c=%u,p=%u,d0=%li,d1=%li,v=%u",
            axis, internal::cal_id, k, (long)c_diff[0], (long)c_diff[1], c_invalid);

        if (c_invalid) {
            // when even just a sample is invalid, we likely have skipped or have a false centroid:
            // no point in retrying, mark the calibration as an instant failure
            internal::home_unstable = true;
            SERIAL_ECHOLNPAIR("home calibration measure ", k, " (", seq[A_AXIS], ",", seq[B_AXIS],
                ") invalid A:", c_diff[A_AXIS], " B:", c_diff[B_AXIS],
                " with origin A:", o_int[A_AXIS], " B:", o_int[B_AXIS]);
            return false;
        }

        if (c_unstable) {
            // mark/log the calibration as unstable, but accept it silently
            internal::home_unstable = true;
            SERIAL_ECHOLNPAIR("home calibration measure ", k, " (", seq[A_AXIS], ",", seq[B_AXIS],
                ") unstable A:", data.c_dist[A_AXIS], " B:", data.c_dist[B_AXIS],
                " with origin A:", origin[A_AXIS], " B:", origin[B_AXIS]);
        }
    }

    // explicitly plan a return to zero to leave origin unchanged as required by the rest of homing
    plan_corexy_abgrid_move(origin_steps, { 0, 0 }, fr_mm_s);

    // recompute the diagonal distance in mm from the computed origin cycles to the endstop
    distance = origin_to_distance(axis, origin);

    metric_record_custom(&metric_phxy_orig, ",a=%u,t=\"d\" c=%u,d0=%.3f,d1=%.3f",
        axis, internal::cal_id, (double)distance[0], (double)distance[1]);

    SERIAL_ECHOLNPAIR("home grid origin A:", origin[A_AXIS], " B:", origin[B_AXIS]);
    return true;
}

bool corexy_rehome_xy(float fr_mm_s) {
    // enable endstops locally
    const bool endstops_enabled = endstops.is_enabled();
    ScopeGuard endstop_restorer([&]() {
        endstops.enable(endstops_enabled);
    });
    endstops.enable(true);

    if (ENABLED(HOME_Y_BEFORE_X)) {
        if (!homeaxis(Y_AXIS, fr_mm_s, false, nullptr, false)) {
            return false;
        }
    }
    if (!homeaxis(X_AXIS, fr_mm_s, false, nullptr, false)) {
        return false;
    }
    if (DISABLED(HOME_Y_BEFORE_X)) {
        if (!homeaxis(Y_AXIS, fr_mm_s, false, nullptr, false)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Rehome, move into position, align to phase and return current position
 * @param origin_pos Final/current home position
 * @param origin_steps Final/current stepper position
 * @param fr_mm_s Service move feedrate
 * @param rehome If true, also perform initial home
 */
static bool corexy_rehome_and_phase(MachinePosXYZE &origin_pos, ab_steps_t &origin_steps, float fr_mm_s, bool rehome) {
    // ignore starting position if requested, otherwise assume to be already homed
    if (rehome) {
        if (!corexy_rehome_xy(fr_mm_s)) {
            return false;
        }
    }

    // reposition parallel to the origin
    {
        auto pos = current_machine_position();
        pos.x = (base_home_pos(X_AXIS) - XY_HOMING_ORIGIN_OFFSET * X_HOME_DIR);
        pos.y = (base_home_pos(Y_AXIS) - XY_HOMING_ORIGIN_OFFSET * Y_HOME_DIR);
        line_to_machine_pos(pos, fr_mm_s);
        planner.synchronize();
    }

    // this position will become our reference for the rest of the home, and might not be exact or
    // actually reached (due to the above move being discarded or optimized). We don't care however,
    // as we additionally want to lose any current fractional step, so disregard the plan and
    // reconstuct our current position
    planner.reset_position();
    origin_pos = planner.get_machine_position_mm();
    PreciseStepping::reset_from_halt(false);

    // align both motors to a full phase
    stepper_wait_for_standstill(_BV(A_AXIS) | _BV(B_AXIS));
    origin_steps[A_AXIS] = (stepper.position(A_AXIS) + phase_backoff_steps(A_AXIS));
    origin_steps[B_AXIS] = (stepper.position(B_AXIS) + phase_backoff_steps(B_AXIS));

    // sanity checks: don't remove these! Issues in repositioning are a result of planner/stepper
    // calculation issues which will show up elsewhere and are NOT just mechanical issues. We need
    // step-accuracy while homing! ask @wavexx when in doubt regarding these
    plan_corexy_raw_move(origin_steps, fr_mm_s);
    const ab_steps_t raw_move_diff = {
        stepper.position(A_AXIS) - origin_steps[A_AXIS],
        stepper.position(B_AXIS) - origin_steps[B_AXIS]
    };
    if (raw_move_diff[A_AXIS] != 0 || raw_move_diff[B_AXIS] != 0) {
        if (planner.draining()) {
            return false;
        }
        SERIAL_ECHOLN("raw move failed");
        SERIAL_ECHOLNPAIR("diff A:", raw_move_diff[A_AXIS], " B:", raw_move_diff[B_AXIS]);
        bsod("raw move didn't reach requested position");
    }

    stepper_wait_for_standstill(_BV(A_AXIS) | _BV(B_AXIS));
    if (!phase_aligned(A_AXIS) || !phase_aligned(B_AXIS)) {
        if (planner.draining()) {
            return false;
        }
        SERIAL_ECHOLN("phase alignment failed");
        SERIAL_ECHOLNPAIR("phase A:", axis_mscnt(A_AXIS), " B:", axis_mscnt(B_AXIS));
        bsod("phase alignment failed");
    }

    return true;
}

#if HAS_TRINAMIC && defined(XY_HOMING_MEASURE_SENS_MIN)
static bool measure_calibrate_walk(float &score, AxisEnum measured_axis,
    const ab_steps_t origin_steps, const float fr_mm_s, const measure_axis_params &params) {
    // prepare for repeated measurements
    const AxisEnum other_axis = (measured_axis == B_AXIS ? A_AXIS : B_AXIS);
    MeasurementGuard setup_guard(other_axis);
    ++internal::probe_id;

    // calculate maximum reliable number of cycles to move towards the endstop
    constexpr AxisEnum walk_axis = X_HOME_DIR == Y_HOME_DIR ? X_AXIS : Y_AXIS;
    constexpr float walk_dist = XY_HOMING_ORIGIN_OFFSET - axis_home_max_diff(walk_axis) * 2;
    static_assert(walk_dist >= 0);
    const size_t walk_cycles = static_cast<size_t>(std::floor(walk_dist / (phase_cycle_steps(walk_axis) * planner.mm_per_step[walk_axis] * std::numbers::sqrt2_v<float>)));
    const size_t walk_period = walk_cycles * 2;
    const size_t measure_probes = std::max<size_t>(walk_period, XY_HOMING_ORIGIN_BUMP_RETRIES * 2);
    debug_assert(measure_probes >= 3 && measure_probes < 128);

    // absolute measure limit distances
    static_assert(XY_HOMING_ORIGIN_OFFSET > axis_home_max_diff(walk_axis) * 2);
    constexpr AxisEnum walk_ortho_axis = walk_axis == X_AXIS ? Y_AXIS : X_AXIS;
    constexpr int32_t measure_min_dist_mm = static_cast<int32_t>((XY_HOMING_ORIGIN_OFFSET - axis_home_max_diff(walk_ortho_axis) * 2) * std::numbers::sqrt2_v<float>);
    constexpr int32_t measure_max_dist_mm = (static_cast<int32_t>(XY_HOMING_ORIGIN_OFFSET) * 4);
    const int32_t measure_min_dist = static_cast<int32_t>(measure_min_dist_mm / planner.mm_per_step[measured_axis]);
    const int32_t measure_max_dist = static_cast<int32_t>(measure_max_dist_mm / planner.mm_per_step[measured_axis]);
    const int32_t measure_dir = (measured_axis == B_AXIS ? -X_HOME_DIR : -Y_HOME_DIR);

    score = 0.f;
    constexpr float score_exp = 3.f;
    float score_acc = 0;

    for (int32_t a_dir = 1; a_dir >= -1; a_dir -= 2) {
        // start from the lowest possible probing position, then zig-zag along the centerpoint to
        // test for weaker positions in the holding rotor and take those into account in addition to
        // testing repeatibility
        int32_t p_steps_buf[measure_probes];
        size_t p_steps_cnt = 0;
        float p_dist;
        int32_t p_steps;

        for (size_t probe = 0; probe != measure_probes; ++probe) {
            const int32_t cycle = probe / walk_period + (a_dir >= 0 ? 0 : 1);
            const int32_t n = probe % walk_period;
            const int32_t d = -int32_t(walk_cycles) + (cycle % 2 ? walk_period - n : n);

            const ab_steps_t temp_origin = plan_corexy_abgrid_move(origin_steps, { d * a_dir, d }, fr_mm_s);
            if (planner.draining()) {
                return false;
            }

            const bool valid = measure_axis_distance(measured_axis, temp_origin,
                measure_max_dist * measure_dir * a_dir, p_steps, p_dist, fr_mm_s, params);
            if (!valid) {
                if (planner.draining()) {
                    return false;
                }
            } else {
                p_steps = abs(p_steps);
                if (p_steps >= measure_min_dist && p_steps <= measure_max_dist) {
                    p_steps_buf[p_steps_cnt++] = p_steps;
                }
            }
        }

        if (p_steps_cnt >= 3) {
            // calculate a score based on central phase deviation independently per-direction
            std::sort(&p_steps_buf[0], &p_steps_buf[p_steps_cnt]);
            const int32_t steps_med = p_steps_buf[p_steps_cnt / 2];
            for (size_t i = 0; i != p_steps_cnt; ++i) {
                const int32_t p_off = abs(steps_med - p_steps_buf[i]) * 4 / phase_cycle_steps(measured_axis);
                const float p_score = 1.f / std::pow(1.f + p_off, score_exp);
                score_acc += p_score;
            }
        }
    }

    // normalize the score by absolute maximum
    score = score_acc / (measure_probes * 2 * std::pow(2.f, score_exp));

    return true;
}

static bool measure_calibrate_sens(CoreXYHomeTMCSens &calibrated_sens,
    AxisEnum measured_axis, const float fr_mm_s) {
    measure_axis_params params = measure_axis_defaults(measured_axis);
    bool rehome = false; // initial home state

    // limits are inclusive
    static_assert(XY_HOMING_MEASURE_SENS_MAX > XY_HOMING_MEASURE_SENS_MIN);
    constexpr size_t slots = (XY_HOMING_MEASURE_SENS_MAX - XY_HOMING_MEASURE_SENS_MIN) + 1;
    static_assert(slots > 1 && slots < 16);
    pair<int8_t, float> scores[slots];
    size_t score_cnt = 0;

    for (int8_t sens = XY_HOMING_MEASURE_SENS_MIN; sens <= XY_HOMING_MEASURE_SENS_MAX; ++sens) {
        MachinePosXYZE origin_pos;
        ab_steps_t origin_steps;

        // reposition parallel to the origin to our probing point
        if (!corexy_rehome_and_phase(origin_pos, origin_steps, fr_mm_s, rehome)) {
            return false;
        }

        // adjust sensitivity and calculate score
        params.sensitivity = sens;
        float score;
        if (!measure_calibrate_walk(score, measured_axis, origin_steps, fr_mm_s, params)) {
            if (planner.draining()) {
                return false;
            }
        } else {
            scores[score_cnt].first = sens;
            scores[score_cnt].second = score;
            ++score_cnt;

            metric_record_custom(&metric_phxy_sens, ",a=%u,sens=%i s=%.4f,f=%.2f,c=%u",
                measured_axis, sens, (double)score, (double)params.feedrate, params.current);
        }

        // always rehome to ignore any undetected skip
        rehome = true;
    }
    if (!score_cnt) {
        return false;
    }

    // pick the best result
    size_t best_idx = 0;
    SERIAL_ECHOLN("sensitivity calibration");
    for (size_t i = 0; i != score_cnt; ++i) {
        SERIAL_ECHOLNPAIR(" sens:", scores[i].first, " score:", scores[i].second);
        if (scores[i].second > scores[best_idx].second) {
            best_idx = i;
        }
    }
    SERIAL_ECHOLNPAIR(" selected:", scores[best_idx].first);

    // we currently only calibrate sensitivity, but save all effective parameters
    calibrated_sens.feedrate = params.feedrate;
    calibrated_sens.current = params.current;
    calibrated_sens.sensitivity = scores[best_idx].first;
    calibrated_sens.score = scores[best_idx].second;
    return true;
}

bool corexy_sens_calibrate(const float fr_mm_s) {
    const AxisEnum measured_axis = (X_HOME_DIR == Y_HOME_DIR ? B_AXIS : A_AXIS);

    // finish previous moves and disable main endstop/crash recovery handling
    planner.synchronize();
    #if HAS_CRASH_DETECTION()
    crash_s.not_for_replay();
    Crash_Temporary_Deactivate ctd;
    #endif

    // disable endstops locally
    const bool endstops_enabled = endstops.is_enabled();
    ScopeGuard endstop_restorer([&]() {
        endstops.enable(endstops_enabled);
    });
    endstops.not_homing();

    SERIAL_ECHOLN("recalibrating homing sensitivity");

    CoreXYHomeTMCSens calibrated_sens;
    if (!measure_calibrate_sens(calibrated_sens, measured_axis, fr_mm_s)) {
        SERIAL_ECHOLNPAIR("home sensitivity calibration failed");
        return false;
    }

    config_store().corexy_home_tmc_sens.set(calibrated_sens);
    return true;
}

bool corexy_sens_is_calibrated() {
    const CoreXYHomeTMCSens calibrated_sens = config_store().corexy_home_tmc_sens.get();
    return !calibrated_sens.uninitialized();
}
#endif

/// Refine home origin precisely on core-XY.
bool corexy_home_refine(float fr_mm_s, CoreXYCalibrationMode mode) {
    const AxisEnum measured_axis = (X_HOME_DIR == Y_HOME_DIR ? B_AXIS : A_AXIS);

    // finish previous moves and disable main endstop/crash recovery handling
    planner.synchronize();
#if HAS_CRASH_DETECTION()
    crash_s.not_for_replay();
    Crash_Temporary_Deactivate ctd;
#endif

    // disable endstops locally
    const bool endstops_enabled = endstops.is_enabled();
    ScopeGuard endstop_restorer([&]() {
        endstops.enable(endstops_enabled);
    });
    endstops.not_homing();

    // reset previous home state
    internal::home_unstable = false;
    ++internal::refine_id;

    // reposition parallel to the origin to our probing point
    MachinePosXYZE origin_pos;
    ab_steps_t origin_steps;
    if (!corexy_rehome_and_phase(origin_pos, origin_steps, fr_mm_s, false)) {
        return false;
    }

    // calibrate origin if not done already
    CoreXYGridOrigin calibrated_origin = config_store().corexy_grid_origin.get();
    if ((mode == CoreXYCalibrationMode::force)
        || ((mode == CoreXYCalibrationMode::on_demand) && calibrated_origin.uninitialized())) {
        SERIAL_ECHOLN("recalibrating home origin");
        ui.status_printf_P(0, "Recalibrating home. Printer may vibrate and be noisier.");
        ++internal::cal_id;

        xy_pos_t origin, distance;
        if (!measure_origin_multipoint(measured_axis, origin_steps, origin, distance, fr_mm_s)) {
            SERIAL_ECHOLNPAIR("home origin calibration failed");
            return false;
        }

        // origin hysteresis: when the origin phase itself falls within the unstable region, anchor
        // the phase to the previous calibration, keeping the stored origin/distance consistent.
        if (!calibrated_origin.uninitialized()) {
            bool valid = true;
            LOOP_XY(axis) {
                const float diff = origin[axis] - calibrated_origin.origin[axis];
                if (fabsf(diff - roundf(diff)) >= unstable_phase_threshold) {
                    // phase moved too much on either axis: ignore previous calibration entirely
                    valid = false;
                    break;
                }
            }
            if (valid) {
                ab_grid_t o_shift = { 0, 0 };
                bool anchor = false;
                LOOP_XY(axis) {
                    const float frac = origin[axis] - floorf(origin[axis]);
                    if (fabsf(frac - 0.5f) < unstable_phase_threshold / 2) {
                        // reuse the previous phase, keep the newly measured integer cycle
                        const float diff = roundf(origin[axis] - calibrated_origin.origin[axis]);
                        o_shift[axis] = static_cast<int32_t>(diff);
                        origin[axis] = calibrated_origin.origin[axis] + diff;
                        anchor = true;
                    }
                }
                if (anchor) {
                    // report the shift and update to the new distance
                    distance = origin_to_distance(measured_axis, origin);

                    SERIAL_ECHOLNPAIR("home origin hysteresis shift A:", o_shift[A_AXIS], " B:", o_shift[B_AXIS]);
                    metric_record_custom(&metric_phxy_home, ",t=\"s\" h=%u,s0=%ld,s1=%ld",
                        internal::refine_id, (long)o_shift[0], (long)o_shift[1]);
                }
            }
        }

        LOOP_XY(axis) {
            calibrated_origin.origin[axis] = origin[axis];
            calibrated_origin.distance[axis] = distance[axis];
        }
        config_store().corexy_grid_origin.set(calibrated_origin);
    } else if (calibrated_origin.uninitialized()) {
        // we have no origin, but calibration was explicitly disabled
#if !HAS_TOOLCHANGER()
        bsod("homing precisely without calibrated origin");
#else
        // TODO[BFW-6527]: this is a temporary workaround until home calibration is enforced
        SERIAL_ECHOLN("warning: homing without calibrated origin");
        calibrated_origin.origin[A_AXIS] = 0.f;
        calibrated_origin.origin[B_AXIS] = 0.f;
#endif
    }
    const xy_pos_t calibrated_origin_xy = {
        calibrated_origin.origin[A_AXIS],
        calibrated_origin.origin[B_AXIS]
    };
    metric_record_custom(&metric_phxy_home, ",t=\"o\" h=%u,o0=%.2f,o1=%.3f",
        internal::refine_id, (double)calibrated_origin_xy[0], (double)calibrated_origin_xy[1]);

    // measure from current origin
    xy_pos_t c_dist;
    if (!measure_phase_cycles(measured_axis, { 0, 0 }, c_dist, fr_mm_s)) {
        return false;
    }

    // validate current origin
    const bool home_unstable = point_is_unstable(c_dist, calibrated_origin_xy);
    if (home_unstable) {
        SERIAL_ECHOLNPAIR("home point is unstable");
    }

    const ab_grid_t c_ab = cdist_translate(c_dist, calibrated_origin_xy);
    metric_record_custom(&metric_phxy_home, ",t=\"h\" h=%u,c0=%.3f,c1=%.3f,s=%u",
        internal::refine_id, (double)c_ab[0], (double)c_ab[1], home_unstable);

    // validate from another point in the AB grid
    const ab_grid_t v_ab_off = { -1, 3 };
    plan_corexy_abgrid_move(origin_steps, v_ab_off, fr_mm_s);
    if (planner.draining()) {
        return false;
    }
    xy_pos_t v_c_dist;
    if (!measure_phase_cycles(measured_axis, v_ab_off, v_c_dist, fr_mm_s)) {
        return false;
    }

    const ab_grid_t v_c_ab = cdist_translate(v_c_dist, calibrated_origin_xy);
    const ab_grid_t v_c_diff = v_c_ab - v_ab_off;
    const bool v_home_unstable = point_is_unstable(v_c_dist, calibrated_origin_xy);
    const bool v_c_invalid = v_c_diff != c_ab;

    metric_record_custom(&metric_phxy_home, ",t=\"v\" h=%u,c0=%.3f,c1=%.3f,s=%u,v=%u",
        internal::refine_id, (double)v_c_ab[0], (double)v_c_ab[1], v_home_unstable, v_c_invalid);

    if (v_c_invalid) {
        internal::home_unstable = true;
        SERIAL_ECHOLNPAIR("home validation point is invalid");
        return false;
    }
    if (v_home_unstable) {
        SERIAL_ECHOLNPAIR("home validation point is unstable");
    }
    if (home_unstable && v_home_unstable) {
        // mark home as unstable only if both points are
        internal::home_unstable = true;
    }

    // move back to origin
    plan_corexy_raw_move(origin_steps, fr_mm_s);
    if (planner.draining()) {
        return false;
    }

    // set machine origin
    const ab_steps_t c_ab_steps {
        c_ab[X_HOME_DIR == Y_HOME_DIR ? A_AXIS : B_AXIS] * phase_cycle_steps(A_AXIS) * -Y_HOME_DIR,
        c_ab[X_HOME_DIR == Y_HOME_DIR ? B_AXIS : A_AXIS] * phase_cycle_steps(B_AXIS) * -X_HOME_DIR
    };

    const MachinePosXY c_mm = corexy_ab_to_xy(c_ab_steps);

    {
        auto target = planner.get_machine_position_mm();
        target.x = c_mm[X_AXIS] + origin_pos[X_AXIS] + XY_HOMING_ORIGIN_OFFSET * X_HOME_DIR;
        target.y = c_mm[Y_AXIS] + origin_pos[Y_AXIS] + XY_HOMING_ORIGIN_OFFSET * Y_HOME_DIR;
        planner.set_machine_position_mm(target);
        set_current_position(to_native_pos(target));
    }

    SERIAL_ECHOLNPAIR("calibrated home cycle A:", c_ab[A_AXIS], " B:", c_ab[B_AXIS]);
    return true;
}

bool corexy_home_is_calibrated() {
    const CoreXYGridOrigin calibrated_origin = config_store().corexy_grid_origin.get();
    return !calibrated_origin.uninitialized();
}

bool corexy_home_is_unstable() {
    const CoreXYGridOrigin calibrated_origin = config_store().corexy_grid_origin.get();
    return calibrated_origin.uninitialized() || internal::home_unstable;
}

void corexy_clear_homing_calibration() {
    config_store().corexy_grid_origin.set_to_default();
}
