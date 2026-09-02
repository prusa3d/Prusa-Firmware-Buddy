#pragma once

// Cut-down stand-in for motion.h. There are no steppers in the tests, so there
// is no position to take back.

#include <core/types.h>

inline xyze_pos_t current_position;

inline void set_current_from_steppers() {}

/// Stands in for the native -> machine conversion. Unset means a flat bed; a
/// test that cares about levelling sets its own correction over the absolute
/// position.
inline float (*stub_mesh_z_correction)(const xyze_pos_t &) = nullptr;

inline MachinePosXYZE to_machine_pos(const xyze_pos_t &pos) {
    MachinePosXYZE result = pos;
    if (stub_mesh_z_correction) {
        result.z += stub_mesh_z_correction(pos);
    }
    return result;
}
