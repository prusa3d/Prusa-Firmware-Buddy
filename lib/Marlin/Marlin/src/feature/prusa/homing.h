#pragma once
#include "config_features.h"
#include "../../core/types.h"
#include "../../module/planner.h"

#ifndef Z_HOMING_HEIGHT
    #define Z_HOMING_HEIGHT 0
#endif

struct workspace_xyz_t {
    xyz_pos_t position_shift;
    xyz_pos_t home_offset;
};

// electrical motor current
struct el_current_xyz_t {
    uint16_t x, y, z;
};

// Reset settings influencing homing
// \param no_modifiers         disable skew and MBL
// \param default_acceleration reset acceleration values
// \param default_current      reset motor currents
void homing_reset(
    bool no_modifiers = true,
    bool default_acceleration = true,
    bool default_current = true);

// Resets bed leveling and skew
// \param do_z Set to true if Z will be homed
// \retval true if leveling should be restored after homing
bool disable_modifiers_if(bool condition, bool do_z);

// Restores bed leveling and skew
void enable_modifiers_if(bool condition, bool restore_leveling);

Motion_Parameters reset_acceleration_if(bool condition);
void restore_acceleration_if(bool condition, Motion_Parameters &mp);
el_current_xyz_t reset_current_if(bool condition);
void restore_current_if(bool condition, el_current_xyz_t current);
