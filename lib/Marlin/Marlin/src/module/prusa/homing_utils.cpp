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
            res.position_shift[axis] = NAN;
            res.home_offset[axis] = NAN;
            continue;
        }

        res.position_shift[axis] = position_shift[axis];
        position_shift[axis] = 0;
        res.home_offset[axis] = home_offset[axis];
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
    // #error dead code found by automatic analyses (see BFW-5461)
    leveling_was_active = planner.leveling_active;
    #else
    if (!do_z) {
        leveling_was_active = planner.leveling_active;
    }
    #endif
    set_bed_leveling_enabled(false);
#endif

    sync_plan_position();
    return leveling_was_active;
}

void enable_modifiers_if(bool condition, bool restore_leveling) {
    if (!condition) {
        return;
    }

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
    {
        auto s = planner.user_settings;
        s.max_acceleration_mm_per_s2[X_AXIS] = XY_HOMING_ACCELERATION;
        s.max_acceleration_mm_per_s2[Y_AXIS] = XY_HOMING_ACCELERATION;
    #if HAS_CLASSIC_JERK
        s.max_jerk.set(XY_HOMING_JERK, XY_HOMING_JERK);
    #endif
        planner.apply_settings(s);
    }
#endif
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

    stepperX.rms_current(get_rms_current_ma_x());
    stepperY.rms_current(get_rms_current_ma_y());
    stepperZ.rms_current(get_rms_current_ma_z());
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

HomingResetGuard::HomingResetGuard(bool no_modifiers, bool default_acceleration, bool default_current)
    : no_modifiers(no_modifiers)
    , default_acceleration(default_acceleration)
    , default_current(default_current) {
#if HAS_WORKSPACE_OFFSET
    disable_workspace(true, true, true);
#endif
    restore_leveling = disable_modifiers_if(no_modifiers, false);
    motion_params = reset_acceleration_if(default_acceleration);
    endstops.enable(true); //< Enable endstops for homing moves
    current = reset_current_if(default_current);
}

HomingResetGuard::~HomingResetGuard() {
    restore_current_if(default_current, current);
    restore_acceleration_if(default_acceleration, motion_params);
    enable_modifiers_if(no_modifiers, restore_leveling);
}
