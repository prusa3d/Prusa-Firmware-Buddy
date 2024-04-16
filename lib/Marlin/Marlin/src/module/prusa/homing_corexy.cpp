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
#include "bsod.h"
#include "log.h"

#include "feature/phase_stepping/phase_stepping.hpp"

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
static int16_t phase_per_ustep(const AxisEnum axis) {
    // Originally, we read the microstep configuration from the driver; this no
    // longer make sense with 256 microsteps.
    // Thus, we use the printer defaults instead of stepper_axis(axis).microsteps();
    assert(axis <= AxisEnum::Z_AXIS);
    static const int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };
    return 256 / MICROSTEPS[axis];
};

// TMC full cycle µsteps per Marlin µsteps
static int16_t phase_cycle_steps(const AxisEnum axis) {
    return 1024 / phase_per_ustep(axis);
}

static int16_t axis_mscnt(const AxisEnum axis) {
#if HAS_BURST_STEPPING()
    return phase_stepping::logical_ustep(axis);
#else
    return stepper_axis(axis).MSCNT();
#endif
}

static int16_t phase_backoff_steps(const AxisEnum axis) {
    int16_t effectorBackoutDir; // Direction in which the effector mm coordinates move away from endstop.
    int16_t stepperBackoutDir; // Direction in which the TMC µstep count(phase) move away from endstop.
    switch (axis) {
    case X_AXIS:
        effectorBackoutDir = -X_HOME_DIR;
        stepperBackoutDir = IF_DISABLED(INVERT_X_DIR, -) effectorBackoutDir;
        break;
    case Y_AXIS:
        effectorBackoutDir = -Y_HOME_DIR;
        stepperBackoutDir = IF_DISABLED(INVERT_Y_DIR, -) effectorBackoutDir;
        break;
    default:
        bsod("invalid backoff axis");
    }

    int16_t phaseCurrent = axis_mscnt(axis); // The TMC µsteps(phase) count of the current position
    int16_t phaseDelta = (0 - phaseCurrent) * stepperBackoutDir;
    if (phaseDelta < 0) {
        phaseDelta += 1024;
    }
    return int16_t(phaseDelta / phase_per_ustep(axis)) * effectorBackoutDir;
}

static bool phase_aligned(AxisEnum axis) {
    int16_t phase_cur = axis_mscnt(axis);
    int16_t ustep_max = phase_per_ustep(axis) / 2;
#if HAS_BURST_STEPPING()
    // TODO: temporarily allow for one logical phase of change until motion is step-exact
    ustep_max += phase_per_ustep(axis);
#endif
    return (phase_cur <= ustep_max || phase_cur >= (1024 - ustep_max));
}

/**
 * @brief Part of precise homing.
 * @param origin_steps
 * @param dist
 * @param m_steps
 * @param m_dist
 * @param orig_crash whether crash_s was active before temporarily disabling it
 * @return
 */
static bool measure_b_axis_distance(xy_long_t origin_steps, int32_t dist, int32_t &m_steps, float &m_dist, const bool orig_crash) {
    // full initial position
    xyze_long_t initial_steps = { origin_steps.a, origin_steps.b, stepper.position(C_AXIS), stepper.position(E_AXIS) };
    xyze_pos_t initial_mm;
    corexy_ab_to_xyze(initial_steps, initial_mm);

    // full target position
    xyze_long_t target_steps = { initial_steps.a, initial_steps.b + dist, initial_steps.c, initial_steps.e };
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
    sensorless_t stealth_states = start_sensorless_homing_per_axis(B_AXIS);
    endstops.enable(true);
    plan_raw_move(target_mm, target_pos_msteps, homing_feedrate(B_AXIS));
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
    end_sensorless_homing_per_axis(B_AXIS, stealth_states);

    // move back to starting point
    plan_raw_move(initial_mm, initial_pos_msteps, homing_feedrate(B_AXIS));

    // sanity checks
    if (hit_steps.a != initial_steps.a || initial_steps.a != stepper.position(A_AXIS)) {
        ui.status_printf_P(0, "A_AXIS didn't return");
        homing_failed([]() { fatal_error(ErrCode::ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED); }, orig_crash);
    }
    if (initial_steps.b != stepper.position(B_AXIS)) {
        ui.status_printf_P(0, "B_AXIS didn't return");
        homing_failed([]() { fatal_error(ErrCode::ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED); }, orig_crash);
    }

    // result values
    m_steps = hit_steps.b - initial_steps.b;
    m_dist = hypotf(hit_mm[X_AXIS] - initial_mm[X_AXIS], hit_mm[Y_AXIS] - initial_mm[Y_AXIS]);
    return hit;
}

/**
 * @brief Return struct for measure_phase_cycles.
 */
struct measure_phase_cycles_ret {
    bool output_valid; // c_dist_n are valid and position can be refined
    bool success; // True on success
    measure_phase_cycles_ret(bool output_valid_, bool success_)
        : output_valid(output_valid_)
        , success(success_) {}
};

/**
 * @brief Part of precise homing.
 * @param c_dist_a
 * @param c_dist_b
 * @param orig_crash whether crash_s was active before temporarily disabling it
 * @return output validity and successfulness
 */
static measure_phase_cycles_ret measure_phase_cycles(int32_t &c_dist_a, int32_t &c_dist_b, const bool orig_crash) {
    const int32_t measure_max_dist = (XY_HOMING_ORIGIN_OFFSET * 4) / planner.mm_per_step[B_AXIS];
    xy_long_t origin_steps = { stepper.position(A_AXIS), stepper.position(B_AXIS) };
    const int n = 2;
    xy_long_t m_steps[n] = { -1, -1 };
    xy_pos_t m_dist[n] = { -1.f, -1.f };
    uint8_t retry;
    for (retry = 0; retry != XY_HOMING_ORIGIN_MAX_RETRIES; ++retry) {
        uint8_t slot = retry % n;
        uint8_t slot2 = (retry - 1) % n;

        // measure distance B+/B-
        if (!measure_b_axis_distance(origin_steps, measure_max_dist, m_steps[slot].b, m_dist[slot].b, orig_crash)
            || !measure_b_axis_distance(origin_steps, -measure_max_dist, m_steps[slot].a, m_dist[slot].a, orig_crash)) {
            if (planner.draining()) {
                return { false, true }; // Do not refine position but end succesfully
            } else {
                ui.status_printf_P(0, "Endstop not reached");
                homing_failed([]() { fatal_error(ErrCode::ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED); }, orig_crash);
                return { false, false }; // Homing failed
            }
        }

        // keep signs positive
        m_steps[slot].a = -m_steps[slot].a;
        m_dist[slot].a = -m_dist[slot].a;

        if (ABS(m_dist[slot].a - m_dist[slot2].a) < XY_HOMING_ORIGIN_BUMPS_MAX_ERR
            && ABS(m_dist[slot].b - m_dist[slot2].b) < XY_HOMING_ORIGIN_BUMPS_MAX_ERR) {
            break;
        }
    }
    if (retry == XY_HOMING_ORIGIN_MAX_RETRIES) {
        ui.status_printf_P(0, "Precise refinement failed"); // User is most likely to get this version of ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED
        homing_failed([]() { fatal_error(ErrCode::ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED); }, orig_crash);
        return { false, false }; // Homing failed
    }

    // calculate the absolute cycle coordinates
    const int32_t c_steps_a = phase_cycle_steps(A_AXIS);
    const int32_t c_steps_b = phase_cycle_steps(B_AXIS);

    int32_t d = (m_steps[0].a + m_steps[1].a + m_steps[0].b + m_steps[1].b) / 2;
    c_dist_a = (d + c_steps_a) / (2 * c_steps_a);
    int32_t d_y = d - (m_steps[0].b + m_steps[1].b);
    int32_t d_y2 = abs(d_y) + c_steps_b;
    if (d_y < 0) {
        d_y2 = -d_y2;
    }
    c_dist_b = d_y2 / (2 * c_steps_b);

    if (DEBUGGING(LEVELING)) {
        // measured distance and cycle
        SERIAL_ECHOLNPAIR("home B+ steps 1: ", m_steps[0].a, " 2: ", m_steps[1].a, " cycle:", c_dist_a);
        SERIAL_ECHOLNPAIR("home B- steps 1: ", m_steps[0].b, " 2: ", m_steps[1].b, " cycle:", c_dist_b);
    }
    return { true, true }; // Success
}

static bool wait_for_standstill(uint8_t axis_mask, millis_t max_delay = 150) {
    millis_t timeout = millis() + max_delay;
    for (;;) {
        bool stst = true;
        LOOP_L_N(i, XYZE_N) {
            if (TEST(axis_mask, i)) {
                if (!static_cast<TMC2130Stepper &>(stepper_axis((AxisEnum)i)).stst()) {
                    stst = false;
                    break;
                }
            }
        }
        if (stst) {
            return true;
        }
        if (millis() > timeout || planner.draining()) {
            return false;
        }
        safe_delay(10);
    }
}

LOG_COMPONENT_REF(Marlin);

/**
 * @brief Precise homing on core-XY.
 * @return true on success
 */
bool refine_corexy_origin() {
    // finish previous moves and disable main endstop/crash recovery handling
    planner.synchronize();
    endstops.not_homing();
#if ENABLED(CRASH_RECOVERY)
    Crash_Temporary_Deactivate ctd;
    const bool orig_crash = ctd.get_orig_state();
#else /*ENABLED(CRASH_RECOVERY)*/
    constexpr bool orig_crash = false;
#endif /*ENABLED(CRASH_RECOVERY)*/

    // reposition parallel to the origin
    float fr_mm_s = homing_feedrate(A_AXIS);
    xyze_pos_t origin_tmp = current_position;
    origin_tmp[X_AXIS] = (base_home_pos(X_AXIS) + XY_HOMING_ORIGIN_OFFSET);
    origin_tmp[Y_AXIS] = (base_home_pos(Y_AXIS) + XY_HOMING_ORIGIN_OFFSET);
    planner.buffer_line(origin_tmp, fr_mm_s, active_extruder);
    planner.synchronize();

    // align both motors to a full phase
    wait_for_standstill(_BV(A_AXIS) | _BV(B_AXIS));
    xy_long_t origin_steps = { stepper.position(A_AXIS) + phase_backoff_steps(A_AXIS),
        stepper.position(B_AXIS) + phase_backoff_steps(B_AXIS) };
    plan_corexy_raw_move(origin_steps, fr_mm_s);
    if (stepper.position(A_AXIS) != origin_steps[A_AXIS] || stepper.position(B_AXIS) != origin_steps[B_AXIS]) {
        // This does actually happen.
        // For example, somebody may call planner.quick_stop()
        // while we were waiting in planner.synchronize()
        log_warning(Marlin, "raw move didn't reach requested position");
        return false;
    }

    // sanity checks
    wait_for_standstill(_BV(A_AXIS) | _BV(B_AXIS));
    if (!phase_aligned(A_AXIS) || !phase_aligned(B_AXIS)) {
        if (planner.draining()) {
            return true;
        }

        SERIAL_ECHOLN("phase alignment failed");
        SERIAL_ECHOLNPAIR("phase A:", stepperX.MSCNT(), " B:", stepperY.MSCNT());
        ui.status_printf_P(0, "Phase alignment failed");
        homing_failed([]() { fatal_error(ErrCode::ERR_MECHANICAL_PRECISE_REFINEMENT_FAILED); }, orig_crash);
        return false;
    }

    // increase current of the holding motor
    int32_t orig_cur = stepperX.rms_current();
    float orig_hold = stepperX.hold_multiplier();
    stepperX.rms_current(XY_HOMING_HOLDING_CURRENT_A, 1.);

    // measure from current origin
    int32_t c_dist_a = 0, c_dist_b = 0;
    auto ret = measure_phase_cycles(c_dist_a, c_dist_b, orig_crash);
    if (ret.output_valid) {
        // convert the full cycle back to steps
        xy_long_t c_ab = { c_dist_a * phase_cycle_steps(A_AXIS), c_dist_b * phase_cycle_steps(B_AXIS) };
        xy_pos_t c_mm;
        corexy_ab_to_xy(c_ab, c_mm);
        current_position.x = c_mm[X_AXIS] + origin_tmp[X_AXIS] + XY_HOMING_ORIGIN_SHIFT_X;
        current_position.y = c_mm[Y_AXIS] + origin_tmp[Y_AXIS] + XY_HOMING_ORIGIN_SHIFT_Y;
        planner.set_machine_position_mm(current_position);
    }

    // restore current
    stepperX.rms_current(orig_cur, orig_hold);

    return ret.success;
}
