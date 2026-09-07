/// @file
#include "toolchanger.h"

#include <module/planner.h>
#include "Marlin.h"
#include "timing.h"
#include <bsod/bsod.h>
#include <option/has_power_panic.h>

#if HAS_POWER_PANIC()
    #include <power_panic.hpp>
#endif

void PrusaToolChanger::z_shift(const float diff) {
    if (axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
        MachinePosXYZE target = current_machine_position();
        target.z += diff;
        target.z = std::clamp<float>(target.z, Z_MIN_POS, Z_MAX_POS);
        line_to_machine_pos(target, Z_HOP_FEEDRATE_MM_S);

    } else {
        // Prevent nasty motor skipping
        do_homing_move(Z_AXIS, diff, HOMING_FEEDRATE_INVERTED_Z);
    }

    planner.synchronize();
}

float PrusaToolChanger::calc_z_raise(tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift, bool levelling_active) const {
    if (z_lift == tool_change_lift_t::no_lift) {
        return 0.f;
    }

    float min_z = current_position.z;

    // raise above the printed model
    min_z = std::max(min_z, planner.max_printed_z);

    // avoid the workpiece for parking
    if (z_lift == tool_change_lift_t::full_lift) {
        min_z += toolchange_settings.z_raise;
    }

    // account for clearance in the return move (dock_backoff has no Z return, like no_return)
    if (return_type != tool_return_t::no_return && return_type != tool_return_t::dock_backoff) {
        min_z = std::max(min_z, return_position.z);
    }

    float z_raise = (min_z - current_position.z);
    if (levelling_active) {
        z_raise += get_mbl_z_lift_height();
    }
    return z_raise;
}

// This function confuses the indexer, so it is last in the file
bool PrusaToolChangerUtils::wait(const stdext::inplace_function<bool()> &function, uint32_t timeout_ms, WaitMode mode) {
    auto should_bail = [mode]() {
        switch (mode) {
        case WaitMode::default_mode:
            return planner.draining();
        case WaitMode::bail_on_power_panic:
#if HAS_POWER_PANIC()
            return power_panic::panic_is_active();
#else
            return false;
#endif
        }
        bsod_unreachable();
    };

    uint32_t start_time = ticks_ms();
    bool result = false;
    while (!(result = function()) // Wait for this and remember its state for return
        && !should_bail()
        && (ticks_ms() - start_time) < timeout_ms) { // Timeout
        idle(true);
    }
    return result;
}
