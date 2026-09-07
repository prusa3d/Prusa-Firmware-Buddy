/*
 * CoreXY kinematic transforms (AB stepper space <-> XY machine space).
 *
 * These are pure CoreXY kinematics.
 * They live in a separate TU so they remain available when
 * homing_corexy.cpp (precise homing refinement) is not compiled in.
 */

#include "corexy_transform.hpp"

#include <module/planner.h>

MachinePosXY corexy_ab_to_xy(const ab_steps_t &steps) {
    return MachinePosXY {
        .x = static_cast<float>(steps.a + steps.b) / 2.f * planner.mm_per_step[X_AXIS],
        .y = static_cast<float>(CORESIGN(steps.a - steps.b)) / 2.f * planner.mm_per_step[Y_AXIS],
    };
}

void corexy_ab_to_xyze(const ab_steps_t &steps, MachinePosXYZE &mm) {
    mm.set(corexy_ab_to_xy(steps));
    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
        mm[i] = planner.get_axis_position_mm((AxisEnum)i);
    }
}
