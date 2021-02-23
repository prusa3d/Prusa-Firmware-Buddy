/// crash_recovery.cpp
#include <algorithm>

#include "crash_recovery.hpp"
#include "display.h"
#include "marlin_client.h"
#include "gui.hpp"

#include "../Marlin/src/module/endstops.h"
#include "../../Marlin/src/module/stepper.h"

/// \param machine position of steppers (real position)
void position_backup(abce_long_t &machine) {
    for (int axis = X_AXIS; axis < E_AXIS; ++axis) {
        machine.pos[axis] = Stepper::position((AxisEnum)axis);
    }
    set_current_from_steppers();
    sync_plan_position();
}

void crash_quick_stop() {
    // Drop all queue entries
    Planner::block_buffer_nonbusy
        = Planner::block_buffer_planned
        = Planner::block_buffer_head
        = Planner::block_buffer_tail;

    // Restart the block delay for the first movement - As the queue was
    // forced to empty, there's no risk the ISR will touch this.
    Planner::delay_before_delivering = 100;

#if HAS_SPI_LCD
    // Clear the accumulated runtime
    clear_block_buffer_runtime();
#endif

    stepper.quick_stop();
}

void restore_planner_after_crash(abce_long_t &machine) {
    Stepper::set_position(machine);
    set_current_from_steppers();
    for (int axis = X_AXIS; axis < E_AXIS; ++axis)
        Planner::set_axis_position((AxisEnum)axis, machine.pos[axis]);
}

void homing_start(AxisEnum axis, const bool positive_dir) {
    // reset position variables to avoid wrong motion
    set_current_from_steppers();
    Planner::refresh_positioning();

    endstops.enable(true);
    const int dir = positive_dir ? 1 : -1;
    const float distance = 1.5f * max_length(axis) * dir;
    const feedRate_t real_fr_mm_s = homing_feedrate(axis);

    abce_pos_t target = { planner.get_axis_position_mm(A_AXIS), planner.get_axis_position_mm(B_AXIS), planner.get_axis_position_mm(C_AXIS), planner.get_axis_position_mm(E_AXIS) };
    target.pos[axis] = distance;
    planner.buffer_segment(target, real_fr_mm_s, active_extruder);
}

void homing_finish(AxisEnum axis, const bool positive_dir, const bool reset_position) {
    planner.synchronize();
    /// reset number of crashes
    endstops.validate_homing_move();

    if (reset_position) {
        set_axis_is_at_home(axis);
        sync_plan_position();
        abce_pos_t target;
        target = { planner.get_axis_position_mm(A_AXIS), planner.get_axis_position_mm(B_AXIS), planner.get_axis_position_mm(C_AXIS), planner.get_axis_position_mm(E_AXIS) };
        switch (axis) {
        case X_AXIS:
            target[axis] = positive_dir ? X_MAX_POS : X_MIN_POS;
            break;
        case Y_AXIS:
            target[axis] = positive_dir ? Y_MAX_POS : Y_MIN_POS;
            break;
        default:
            break;
        }
        planner.set_machine_position_mm(target);
    }
    endstops.not_homing();
    planner.synchronize();
    /// set_current_from_steppers(); should be used instead
    current_position.pos[axis] = planner.get_axis_position_mm(axis);
    sync_plan_position();
}

void homing_finish_axis(AxisEnum axis) {
    homing_finish(axis, false, false);
}
