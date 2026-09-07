/**
 * @file
 * @brief Measure length of X and Y axes with ease
 *
 * Non-blocking measurement of X and Y axes. You can choose the direction
 * of the measuring also with quick home before measurement.
 *
 * Usage:
 * Create and object of the class and run start() to run the process.
 * Then call loop regularly to make progress.
 */

#include "measure_axis.h"
#include "../motordriver_util.h"
#include <option/has_crash_detection.h>

#if ENABLED(AXIS_MEASURE)

    #include "crash_recovery.hpp"
    #include "../../module/stepper.h"
    #include "../../module/endstops.h"

    #define _CAN_HOME(A) \
        ((A##_MIN_PIN > -1 && A##_HOME_DIR < 0) || (A##_MAX_PIN > -1 && A##_HOME_DIR > 0))
    #define CAN_HOME_X _CAN_HOME(X)
    #define CAN_HOME_Y _CAN_HOME(Y)

Measure_axis::Measure_axis(bool measure_x, bool measure_y, xy_bool_t invert_dir, feedRate_t fr_mm_s,
    float raise_z, bool no_modifiers, bool default_acceleration, bool default_current)
    : raise_z(raise_z)
    , state_(INIT)
    , invert_dir(invert_dir)
    , do_x(measure_x)
    , do_y(measure_y)
    , no_modifiers(no_modifiers)
    , default_acceleration(default_acceleration)
    , default_current(default_current) {
    if ((!do_x && !do_y) || (!CAN_HOME_X && !CAN_HOME_Y)) {
        state_ = FINISH;
        return;
    }

    if (raise_z < 0) {
        raise_z = Z_HOMING_HEIGHT;
    }

    if (fr_mm_s <= 0) {
        fr.x = homing_feedrate(X_AXIS);
        fr.y = homing_feedrate(Y_AXIS);
    }
}

// TODO: use this for quick_home in Marlin (de-duplicate code)
void Measure_axis::quick_home_start() {
    #if ENABLED(QUICK_HOME)
    // Pretend the current position is 0,0
    current_position.set(0.0f, 0.0f);
    sync_plan_position();

    int axis_home_dir[2];
    axis_home_dir[X_AXIS] = (invert_dir.x ? (-home_dir(X_AXIS))
                                          : home_dir(X_AXIS));

    axis_home_dir[Y_AXIS] = invert_dir.y ? (-home_dir(X_AXIS)) : home_dir(X_AXIS);

    const float speed_ratio = homing_feedrate(X_AXIS) / homing_feedrate(Y_AXIS);
    const float speed_ratio_inv = homing_feedrate(Y_AXIS) / homing_feedrate(X_AXIS);
    const float length_ratio = max_length(X_AXIS) / max_length(Y_AXIS);
    const bool length_r_less_than_speed_r = length_ratio < speed_ratio;

    const float mlx = 1.5f * (length_r_less_than_speed_r ? (max_length(Y_AXIS) * speed_ratio) : (max_length(X_AXIS)));
    const float mly = 1.5f * (length_r_less_than_speed_r ? (max_length(Y_AXIS)) : (max_length(X_AXIS) * speed_ratio_inv));
    const float fr_mm_s = SQRT(sq(homing_feedrate(X_AXIS)) + sq(homing_feedrate(Y_AXIS)));

        #if ENABLED(SENSORLESS_HOMING)
    stealth_states = {
        enable_crash_detection(X_AXIS),
        enable_crash_detection(Y_AXIS),
        false,
        false,
        false,
    };

    sensorless_enable(X_AXIS);
    sensorless_enable(Y_AXIS);
        #endif

    plan_park_move_to(mlx * axis_home_dir[X_AXIS], mly * axis_home_dir[Y_AXIS], current_position.z, fr_mm_s, homing_feedrate(Z_AXIS), Segmented::no);
    #endif
}

void Measure_axis::quick_home_finish() {
    #if ENABLED(QUICK_HOME)
    endstops.validate_homing_move();
    current_position.set(0.0f, 0.0f);
    sync_plan_position();
        #if ENABLED(SENSORLESS_HOMING)
    LOOP_XY(axis)
    end_sensorless_homing_per_axis(AxisEnum(axis), stealth_states);
        #endif
    #endif
}

void Measure_axis::sensorless_enable(AxisEnum axis) {
    // Disable stealthChop if used. Enable diag1 pin on driver.
    #if ENABLED(SENSORLESS_HOMING)
    stealth_states = start_sensorless_homing_per_axis(axis);
    switch (axis) {
    default:
        break;
        #if X_SENSORLESS
    case X_AXIS:
        if (sensitivity.has_value()) {
            stepperX.stall_sensitivity(sensitivity.value().x);
        }
        if (max_period.has_value()) {
            stepperX.stall_max_period(max_period.value().x);
        }
        break;
        #endif
        #if Y_SENSORLESS
    case Y_AXIS:
        if (sensitivity.has_value()) {
            stepperY.stall_sensitivity(sensitivity.value().y);
        }
        if (max_period.has_value()) {
            stepperX.stall_max_period(max_period.value().y);
        }
        break;
        #endif
    }
    #endif // ENABLED(SENSORLESS_HOMING)
}

void Measure_axis::sensorless_disable(AxisEnum axis) {
    // Re-enable stealthChop if used. Disable diag1 pin on driver.
    #if ENABLED(SENSORLESS_HOMING)
    end_sensorless_homing_per_axis(axis, stealth_states);
    #endif // ENABLED(SENSORLESS_HOMING)
}

void Measure_axis::home_back(AxisEnum axis) {
    // FIXME: duplicit code
    #if ENABLED(MOVE_BACK_BEFORE_HOMING)
    const int axis_home_dir = (invert_dir[axis] ? (-home_dir(axis))
                                                : home_dir(axis));
    float dist = (axis_home_dir > 0) ? -MOVE_BACK_BEFORE_HOMING_DISTANCE : MOVE_BACK_BEFORE_HOMING_DISTANCE;
    (void)do_homing_move_axis_rel(axis, dist, fr[axis]);
    #endif
}

void Measure_axis::home_start(AxisEnum axis, bool invert) {
    // FIXME: duplicit code
    const int axis_home_dir = (invert_dir[axis] ^ invert ? (-home_dir(axis))
                                                         : home_dir(axis));
    float dist = 1.5f * max_length(axis) * axis_home_dir;
    home_start_motor_pos[axis] = planner.get_axis_position_mm(axis);
    (void)do_homing_move_axis_rel(axis, dist, fr[axis]);
}

void Measure_axis::save_length(AxisEnum axis) {
    axis_length[axis] = std::abs(home_start_motor_pos[axis] - planner.get_axis_position_mm(axis));
}

void Measure_axis::finish() {
    #if HAS_CRASH_DETECTION()
    crash_s.activate();
    #endif
    sync_plan_position();
    restore_current_if(default_current, current);
    endstops.not_homing();
    restore_acceleration_if(default_acceleration, mp);
    enable_modifiers_if(no_modifiers, leveling);
}

void Measure_axis::state_start() {
    switch (state_) {
    case RAISE_Z: {
        // TODO: raise Z with Z stallguard
        destination = current_position;
        if (axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
            destination.z = std::max(raise_z, destination.z); //< lift at least to raise_z
        } else {
            destination.z += raise_z; //< lift by raise_z because we don't know where the Z is
        }
        const feedRate_t fr_z = homing_feedrate(Z_AXIS);
        plan_park_move_to_xyz(destination.xyz(), fr_z, fr_z, Segmented::yes);
        break;
    }
    case QUICK_HOME_XY:
        if (do_x && do_y) {
            quick_home_start();
        } else {
            state_change(BACK_X);
        }
        break;
    case BACK_X:
        if (!do_x) {
            state_change(BACK_Y);
        } else {
            sensorless_enable(X_AXIS);
            home_back(X_AXIS);
        }
        break;
    case HOME_X:
        home_start(X_AXIS);
        break;
    case MEASURE_X:
        home_start(X_AXIS, true);
        break;
    case BACK_Y:
        if (!do_y) {
            state_change(FINISH);
        } else {
            sensorless_enable(Y_AXIS);
            home_back(Y_AXIS);
        }
        break;
    case HOME_Y:
        home_start(Y_AXIS);
        break;
    case MEASURE_Y:
        home_start(Y_AXIS, true);
        break;
    case FINISH:
        finish();
        break;
    default:
        break;
    }
}

void Measure_axis::state_finish() {
    switch (state_) {
    case WAIT:
        // No need to disable workspace since it's only a shift which does not influences axes' measurement
        leveling = disable_modifiers_if(no_modifiers, false);
        mp = reset_acceleration_if(default_acceleration);
        endstops.enable(true); //< Enable endstops for homing moves
        current = reset_current_if(default_current);
    #if HAS_CRASH_DETECTION()
        crash_s.deactivate();
    #endif
        break;
    case QUICK_HOME_XY:
        if (do_x && do_y) {
            quick_home_finish();
        }
        break;
    case MEASURE_X:
        save_length(X_AXIS);
        sensorless_disable(X_AXIS);
        // don't break
    case HOME_X:
        break;
    case MEASURE_Y:
        save_length(Y_AXIS);
        sensorless_disable(Y_AXIS);
        // don't break
    case HOME_Y:
        break;
    default:
        break;
    }
}

void Measure_axis::loop() {
    switch (state_) {
    case FINISH:
        break;
    default:
        if (!Planner::busy()) {
            state_next();
        }
        break;
    }
}

#endif
