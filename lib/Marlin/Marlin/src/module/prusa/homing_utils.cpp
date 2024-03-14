/**
 * Group of functions to make homing more easier, reliable and flexible.
 */

#include "homing_utils.hpp"
#include "../../feature/bedlevel/bedlevel.h"
#include "../../feature/prusa/crash_recovery.hpp"
#include "../../module/endstops.h"
#include "../stepper.h"
#include <config_store/store_instance.hpp>

#if HAS_WORKSPACE_OFFSET
static workspace_xyz_t disable_workspace(bool do_x, bool do_y, bool do_z) {
    bool changed = false;
    workspace_xyz_t res;

    LOOP_XYZ(axis) {
        if (!((do_x && axis == X_AXIS) || (do_y && axis == Y_AXIS) || (do_z && axis == Z_AXIS))) {
            res.position_shift.pos[axis] = NAN;
            res.home_offset.pos[axis] = NAN;
            continue;
        }

        current_position.pos[axis] = LOGICAL_TO_NATIVE(current_position.pos[axis], axis);
        res.position_shift.pos[axis] = position_shift.pos[axis];
        position_shift.pos[axis] = 0;
        res.home_offset.pos[axis] = home_offset.pos[axis];
        set_home_offset(AxisEnum(axis), 0); //< updates workspace
        changed = true;
    }

    if (changed) {
        sync_plan_position();
    }

    return res;
}
#endif // HAS_WORKSPACE_OFFSET

bool disable_modifiers_if(bool condition, bool do_z) {
    if (!condition) {
        return false;
    }

    bool leveling_was_active = false;
#if HAS_LEVELING
    #if ENABLED(RESTORE_LEVELING_AFTER_G28)
    leveling_was_active = planner.leveling_active;
    #else
    if (!do_z) {
        leveling_was_active = planner.leveling_active;
    }
    #endif
    set_bed_leveling_enabled(false);
#endif

    // Already done by set_bed_leveling_enabled(false)
    // #if ENABLED(SKEW_CORRECTION)
    //   unskew(current_position);
    // #endif

    sync_plan_position();
    return leveling_was_active;
}

void enable_modifiers_if(bool condition, bool restore_leveling) {
    if (!condition) {
        return;
    }

#if ENABLED(SKEW_CORRECTION)
    skew(current_position);
#endif

#if HAS_LEVELING
    if (restore_leveling) {
        set_bed_leveling_enabled(true);
    }
#endif
    sync_plan_position();
}

Motion_Parameters reset_acceleration_if(bool condition) {
    Motion_Parameters mp;
    mp.save();
    if (!condition) {
        return mp;
    }

    mp.reset();

#if ENABLED(IMPROVE_HOMING_RELIABILITY)
    planner.settings.max_acceleration_mm_per_s2[X_AXIS] = XY_HOMING_ACCELERATION;
    planner.settings.max_acceleration_mm_per_s2[Y_AXIS] = XY_HOMING_ACCELERATION;
    #if HAS_CLASSIC_JERK
    planner.max_jerk.set(XY_HOMING_JERK, XY_HOMING_JERK);
    #endif
#endif
    planner.refresh_acceleration_rates();
    remember_feedrate_scaling_off();
    return mp;
}

void restore_acceleration_if(bool condition, Motion_Parameters &mp) {
    if (!condition) {
        return;
    }

    restore_feedrate_and_scaling();
    mp.load();
    planner.refresh_acceleration_rates();
}

el_current_xyz_t reset_current_if(bool condition) {
    el_current_xyz_t curr = { stepperX.rms_current(), stepperY.rms_current(), stepperZ.rms_current() };
    if (!condition) {
        return curr;
    }

    stepperX.rms_current(get_default_rms_current_ma_x());
    stepperY.rms_current(get_default_rms_current_ma_y());
    stepperZ.rms_current(get_default_rms_current_ma_z());
    return curr;
}

void restore_current_if(bool condition, el_current_xyz_t current) {
    if (!condition) {
        return;
    }

    stepperX.rms_current(current.x);
    stepperY.rms_current(current.y);
    stepperZ.rms_current(current.z);
}

void homing_reset(bool no_modifiers, bool default_acceleration, bool default_current) {
#if HAS_WORKSPACE_OFFSET
    disable_workspace(true, true, true);
#endif
    disable_modifiers_if(no_modifiers, false);
    reset_acceleration_if(default_acceleration);
    endstops.enable(true); //< Enable endstops for homing moves
    reset_current_if(default_current);
}
