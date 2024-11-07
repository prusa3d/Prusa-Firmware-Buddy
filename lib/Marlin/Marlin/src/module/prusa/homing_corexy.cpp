/*
 * CoreXY precise homing refinement - implementation
 * TODO: @wavexx: Add some documentation
 */

#include "homing_corexy.hpp"

#include "../planner.h"
#include "../stepper.h"
#include "../endstops.h"

#if ENABLED(CRASH_RECOVERY)
    #include "feature/prusa/crash_recovery.hpp"
#endif

#include <bsod.h>
#include <scope_guard.hpp>
#include <feature/phase_stepping/phase_stepping.hpp>
#include <config_store/store_instance.hpp>

// convert raw AB steps to XY mm
void corexy_ab_to_xy(const xy_long_t &steps, xy_pos_t &mm) {
    float x = static_cast<float>(steps.a + steps.b) / 2.f;
    float y = static_cast<float>(CORESIGN(steps.a - steps.b)) / 2.f;
    mm.x = x * planner.mm_per_step[X_AXIS];
    mm.y = y * planner.mm_per_step[Y_AXIS];
}

// convert raw AB steps to XY mm and position in mini-steps
static void corexy_ab_to_xy(const xy_long_t &steps, xy_pos_t &mm, xy_long_t &pos_msteps) {
    float x = static_cast<float>(steps.a + steps.b) / 2.f;
    float y = static_cast<float>(CORESIGN(steps.a - steps.b)) / 2.f;
    mm.x = x * planner.mm_per_step[X_AXIS];
    mm.y = y * planner.mm_per_step[Y_AXIS];
    pos_msteps.x = LROUND(x * PLANNER_STEPS_MULTIPLIER);
    pos_msteps.y = LROUND(y * PLANNER_STEPS_MULTIPLIER);
}

// convert raw AB steps to XY mm, filling others from current state
void corexy_ab_to_xyze(const xy_long_t &steps, xyze_pos_t &mm) {
    corexy_ab_to_xy(steps, mm);
    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
        mm[i] = planner.get_axis_position_mm((AxisEnum)i);
    }
}

// convert raw AB steps to XY mm and position in mini-steps, filling others from current state
static void corexy_ab_to_xyze(const xy_long_t &steps, xyze_pos_t &mm, xyze_long_t &pos_msteps) {
    pos_msteps = planner.get_position_msteps();
    corexy_ab_to_xy(steps, mm, pos_msteps);
    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
        mm[i] = planner.get_axis_position_mm((AxisEnum)i);
    }
}

static void plan_raw_move(const xyze_pos_t target_mm, const xyze_long_t target_pos, const feedRate_t fr_mm_s) {
    planner._buffer_msteps_raw(target_pos, target_mm, fr_mm_s, active_extruder);
    planner.synchronize();
}

static void plan_corexy_raw_move(const xy_long_t &target_steps_ab, const feedRate_t fr_mm_s) {
    // reconstruct full final position
    xyze_pos_t target_mm;
    xyze_long_t target_pos_msteps;
    corexy_ab_to_xyze(target_steps_ab, target_mm, target_pos_msteps);

    plan_raw_move(target_mm, target_pos_msteps, fr_mm_s);
}

// TMC µsteps(phase) per Marlin µsteps
static constexpr int16_t phase_per_ustep(const AxisEnum axis) {
    // Originally, we read the microstep configuration from the driver; this no
    // longer make sense with 256 microsteps.
    // Thus, we use the printer defaults instead of stepper_axis(axis).microsteps();
    assert(axis <= AxisEnum::Z_AXIS);
    static const int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };
    return 256 / MICROSTEPS[axis];
};

// TMC full cycle µsteps per Marlin µsteps
static constexpr int16_t phase_cycle_steps(const AxisEnum axis) {
    return 1024 / phase_per_ustep(axis);
}

static int16_t axis_mscnt(const AxisEnum axis) {
#if HAS_PHASE_STEPPING()
    return phase_stepping::logical_ustep(axis);
#else
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

    int16_t phaseCurrent = axis_mscnt(axis); // The TMC µsteps(phase) count of the current position
    int16_t phaseDelta = ((stepperCountDir < 0) == (effectorBackoutDir < 0) ? phaseCurrent : 1024 - phaseCurrent);
    int16_t phasePerStep = phase_per_ustep(axis);
    return int16_t((phaseDelta + phasePerStep / 2) / phasePerStep) * effectorBackoutDir;
}

static bool phase_aligned(AxisEnum axis) {
    int16_t phase_cur = axis_mscnt(axis);
    int16_t ustep_max = phase_per_ustep(axis) / 2;
    return (phase_cur <= ustep_max || phase_cur >= (1024 - ustep_max));
}

/**
 * @brief Part of precise homing.
 * @param origin_steps
 * @param dist
 * @param m_steps
 * @param m_dist
 * @return True on success
 */
static bool measure_axis_distance(AxisEnum axis, xy_long_t origin_steps, int32_t dist, int32_t &m_steps, float &m_dist) {
    // full initial position
    xyze_long_t initial_steps = { origin_steps.a, origin_steps.b, stepper.position(C_AXIS), stepper.position(E_AXIS) };
    xyze_pos_t initial_mm;
    corexy_ab_to_xyze(initial_steps, initial_mm);

    // full target position
    xyze_long_t target_steps = initial_steps;
    target_steps[axis] += dist;

    xyze_pos_t target_mm;
    xyze_long_t target_pos_msteps;
    corexy_ab_to_xy(target_steps, target_mm, target_pos_msteps);
    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
        target_mm[i] = initial_mm[i];
    }
    xyze_long_t initial_pos_msteps = planner.get_position_msteps();
    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
        target_pos_msteps[i] = initial_pos_msteps[i];
    }

    // move towards the endstop
    sensorless_t stealth_states = start_sensorless_homing_per_axis(axis);
    endstops.enable(true);
    plan_raw_move(target_mm, target_pos_msteps, homing_feedrate(axis));
    uint8_t hit = endstops.trigger_state();
    endstops.not_homing();

    xyze_long_t hit_steps;
    xyze_pos_t hit_mm;
    if (hit) {
        // resync position from steppers to get hit position
        endstops.hit_on_purpose();
        planner.reset_position();
        hit_steps = { stepper.position(A_AXIS), stepper.position(B_AXIS), stepper.position(C_AXIS), stepper.position(E_AXIS) };
        corexy_ab_to_xyze(hit_steps, hit_mm);
    } else {
        hit_steps = target_steps;
        hit_mm = target_mm;
    }
    end_sensorless_homing_per_axis(axis, stealth_states);

    // move back to starting point
    plan_raw_move(initial_mm, initial_pos_msteps, homing_feedrate(axis));

    // sanity checks
    AxisEnum fixed_axis = (axis == B_AXIS ? A_AXIS : B_AXIS);
    if (hit_steps[fixed_axis] != initial_steps[fixed_axis] || initial_steps[fixed_axis] != stepper.position(fixed_axis)) {
        bsod("fixed axis moved unexpectedly");
    }

    if (initial_steps[axis] != stepper.position(axis)) {
        if (!planner.draining()) {
            bsod("measured axis didn't return");
        }
        return false;
    }

    // result values
    m_steps = hit_steps[axis] - initial_steps[axis];
    m_dist = hypotf(hit_mm[X_AXIS] - initial_mm[X_AXIS], hit_mm[Y_AXIS] - initial_mm[Y_AXIS]);
    return hit;
}

/**
 * @brief Part of precise homing.
 * @param axis Physical axis to measure
 * @param c_dist AB cycle distance from the endstop
 * @param m_dist 1/2 distance from the endstop (mm)
 * @return True on success
 */
static bool measure_phase_cycles(AxisEnum axis, xy_pos_t &c_dist, xy_pos_t &m_dist) {
    // increase current of the holding motor
    AxisEnum other_axis = (axis == B_AXIS ? A_AXIS : B_AXIS);
    auto &other_stepper = stepper_axis(other_axis);

    int32_t orig_cur = other_stepper.rms_current();
    float orig_hold = other_stepper.hold_multiplier();
    other_stepper.rms_current(XY_HOMING_HOLDING_CURRENT, 1.);

    ScopeGuard current_restorer([&]() {
        other_stepper.rms_current(orig_cur, orig_hold);
    });

    const int32_t measure_max_dist = (XY_HOMING_ORIGIN_OFFSET * 4) / planner.mm_per_step[axis];
    xy_long_t origin_steps = { stepper.position(A_AXIS), stepper.position(B_AXIS) };
    constexpr int probe_n = 2; // note the following code assumes always two probes per retry
    xy_long_t p_steps[probe_n];
    xy_pos_t p_dist[probe_n] = { -XY_HOMING_ORIGIN_BUMPS_MAX_ERR, -XY_HOMING_ORIGIN_BUMPS_MAX_ERR };

    uint8_t retry;
    for (retry = 0; retry != XY_HOMING_ORIGIN_MAX_RETRIES; ++retry) {
        uint8_t slot0 = retry % probe_n;
        uint8_t slot1 = (retry + 1) % probe_n;

        // measure distance B+/B-
        if (!measure_axis_distance(axis, origin_steps, measure_max_dist, p_steps[slot1][1], p_dist[slot1][1])
            || !measure_axis_distance(axis, origin_steps, -measure_max_dist, p_steps[slot1][0], p_dist[slot1][0])) {
            if (!planner.draining()) {
                ui.status_printf_P(0, "Endstop not reached");
            }
            return false;
        }

        // keep signs positive
        p_steps[slot1][0] = -p_steps[slot1][0];
        p_dist[slot1][0] = -p_dist[slot1][0];

        if (ABS(p_dist[slot0][0] - p_dist[slot1][0]) < XY_HOMING_ORIGIN_BUMPS_MAX_ERR
            && ABS(p_dist[slot0][1] - p_dist[slot1][1]) < XY_HOMING_ORIGIN_BUMPS_MAX_ERR) {
            break;
        }
    }
    if (retry == XY_HOMING_ORIGIN_MAX_RETRIES) {
        ui.status_printf_P(0, "Precise refinement failed");
        return false;
    }

    // calculate the absolute cycle coordinates
    float d1 = (p_steps[0][0] + p_steps[1][0]) / 2.f;
    float d2 = (p_steps[0][1] + p_steps[1][1]) / 2.f;
    float d = d1 + d2;
    float a = d / 2.;
    float b = d1 - a;

    c_dist[other_axis] = a / float(phase_cycle_steps(A_AXIS));
    c_dist[axis] = b / float(phase_cycle_steps(B_AXIS));

    m_dist[0] = (p_dist[0][0] + p_dist[1][0]) / 2.f;
    m_dist[1] = (p_dist[0][1] + p_dist[1][1]) / 2.f;

    if (DEBUGGING(LEVELING)) {
        // measured distance and cycle
        SERIAL_ECHOLNPAIR("home ", physical_axis_codes[axis], "+ steps 0:", p_steps[0][1], " 1:", p_steps[1][1],
            " cycle ", physical_axis_codes[other_axis], ":", c_dist[other_axis], " mm:", m_dist[1]);
        SERIAL_ECHOLNPAIR("home ", physical_axis_codes[axis], "- steps 0:", p_steps[0][0], " 1:", p_steps[1][0],
            " cycle ", physical_axis_codes[axis], ":", c_dist[axis], " mm:", m_dist[0]);
    }
    return true;
}

bool measure_origin_multipoint(AxisEnum axis, const xy_long_t &origin_steps, xy_pos_t &origin, xy_pos_t &distance) {
    // scramble probing sequence to improve belt redistribution when estimating the centroid
    static constexpr int8_t point_sequence[][2] = {
        { 1, 0 },
        { -1, 0 },
        { 0, 1 },
        { 0, -1 },
        { -1, -1 },
        { 1, 1 },
        { 1, -1 },
        { -1, 1 },
        { 0, 0 },
    };

    const float fr_mm_s = homing_feedrate(A_AXIS);
    xy_pos_t c_acc = { 0, 0 };
    xy_pos_t m_acc = { 0, 0 };

    for (const auto &seq : point_sequence) {
        xy_long_t point_steps = {
            origin_steps[A_AXIS] + phase_cycle_steps(A_AXIS) * seq[A_AXIS],
            origin_steps[B_AXIS] + phase_cycle_steps(B_AXIS) * seq[B_AXIS]
        };

        plan_corexy_raw_move(point_steps, fr_mm_s);

        xy_pos_t c_dist, m_dist;
        if (!measure_phase_cycles(axis, c_dist, m_dist)) {
            return false;
        }

        c_acc += c_dist;
        m_acc += m_dist;
    }
    origin = c_acc / float(std::size(point_sequence));
    distance = m_acc / float(std::size(point_sequence));

    if (DEBUGGING(LEVELING)) {
        SERIAL_ECHOLNPAIR("home grid origin A:", origin[A_AXIS], " B:", origin[B_AXIS]);
    }
    return true;
}

// Refine home origin precisely on core-XY.
bool refine_corexy_origin(CoreXYCalibrationMode mode) {
    // finish previous moves and disable main endstop/crash recovery handling
    planner.synchronize();
    endstops.not_homing();
#if ENABLED(CRASH_RECOVERY)
    Crash_Temporary_Deactivate ctd;
#endif /*ENABLED(CRASH_RECOVERY)*/

    // reposition parallel to the origin
    float fr_mm_s = homing_feedrate(A_AXIS);
    xyze_pos_t origin_tmp = current_position;
    origin_tmp[X_AXIS] = (base_home_pos(X_AXIS) - XY_HOMING_ORIGIN_OFFSET * X_HOME_DIR);
    origin_tmp[Y_AXIS] = (base_home_pos(Y_AXIS) - XY_HOMING_ORIGIN_OFFSET * Y_HOME_DIR);
    planner.buffer_line(origin_tmp, fr_mm_s, active_extruder);
    planner.synchronize();

    // align both motors to a full phase
    stepper_wait_for_standstill(_BV(A_AXIS) | _BV(B_AXIS));
    xy_long_t origin_steps = {
        stepper.position(A_AXIS) + phase_backoff_steps(A_AXIS),
        stepper.position(B_AXIS) + phase_backoff_steps(B_AXIS)
    };

    // sanity checks: don't remove these! Issues in repositioning are a result of planner/stepper
    // calculation issues which will show up elsewhere and are NOT just mechanical issues. We need
    // step-accuracy while homing! ask @wavexx when in doubt regarding these
    plan_corexy_raw_move(origin_steps, fr_mm_s);
    xy_long_t raw_move_diff = {
        stepper.position(A_AXIS) - origin_steps[A_AXIS],
        stepper.position(B_AXIS) - origin_steps[B_AXIS]
    };
    if (raw_move_diff[A_AXIS] != 0 || raw_move_diff[B_AXIS] != 0) {
        if (planner.draining()) {
            return true;
        }
        SERIAL_ECHOLN("raw move failed");
        SERIAL_ECHOLNPAIR("diff A:", raw_move_diff[A_AXIS], " B:", raw_move_diff[B_AXIS]);
        bsod("raw move didn't reach requested position");
    }

    stepper_wait_for_standstill(_BV(A_AXIS) | _BV(B_AXIS));
    if (!phase_aligned(A_AXIS) || !phase_aligned(B_AXIS)) {
        if (planner.draining()) {
            return true;
        }
        SERIAL_ECHOLN("phase alignment failed");
        SERIAL_ECHOLNPAIR("phase A:", axis_mscnt(A_AXIS), " B:", axis_mscnt(B_AXIS));
        bsod("phase alignment failed");
    }

    AxisEnum measured_axis = (X_HOME_DIR == Y_HOME_DIR ? B_AXIS : A_AXIS);

    // calibrate if not done already
    CoreXYGridOrigin calibrated_origin = config_store().corexy_grid_origin.get();
    if ((mode == CoreXYCalibrationMode::Force)
        || ((mode == CoreXYCalibrationMode::OnDemand) && calibrated_origin.uninitialized())) {
        SERIAL_ECHOLN("recalibrating homing origin");
        ui.status_printf_P(0, "Recalibrating home. Printer may vibrate and be noisier.");

        xy_pos_t origin, distance;
        if (!measure_origin_multipoint(measured_axis, origin_steps, origin, distance)) {
            return false;
        }

        LOOP_XY(axis) {
            calibrated_origin.origin[axis] = origin[axis];
            calibrated_origin.distance[axis] = distance[axis];
        }
        config_store().corexy_grid_origin.set(calibrated_origin);
    }

    // measure from current origin
    xy_pos_t c_dist, _;
    if (!measure_phase_cycles(measured_axis, c_dist, _)) {
        return false;
    }

    // offset and convert the AB cycle coordinates back to steps
    xy_long_t c_ab;
    LOOP_XY(axis) {
        float o_fract = calibrated_origin.origin[axis];
        long o_int = long(roundf(o_fract));
        c_ab[axis] = long(roundf(c_dist[axis] - o_fract)) + o_int;
    };

    xy_long_t c_ab_steps = { c_ab[A_AXIS] * phase_cycle_steps(A_AXIS), c_ab[B_AXIS] * phase_cycle_steps(B_AXIS) };
    xy_pos_t c_mm;
    corexy_ab_to_xy(c_ab_steps, c_mm);
    current_position.x = c_mm[X_AXIS] + origin_tmp[X_AXIS] - XY_HOMING_ORIGIN_SHIFT_X * X_HOME_DIR;
    current_position.y = c_mm[Y_AXIS] + origin_tmp[Y_AXIS] - XY_HOMING_ORIGIN_SHIFT_Y * Y_HOME_DIR;
    planner.set_machine_position_mm(current_position);

    if (DEBUGGING(LEVELING)) {
        SERIAL_ECHOLNPAIR("calibrated home cycle A:", c_ab[A_AXIS], " B:", c_ab[B_AXIS]);
    }

    return true;
}
