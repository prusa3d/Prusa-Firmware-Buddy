
#include "PrusaGcodeSuite.hpp"
#include <feature/emergency_stop/emergency_stop.hpp>
#include <common/marlin_server.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M9202: Wait for emergency stop conditions to pass.
 *
 * Can be called even outside of emergency stop conditions (in which case it'll
 * just do nothing).
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M9201
 *
 */
void PrusaGcodeSuite::M9202() {
    if (!buddy::emergency_stop().do_stop) {
        return;
    }

    marlin_server::set_warning(WarningType::DoorOpen, PhasesWarning::DoorOpen);

    planner.synchronize();
    const auto old = current_position;
    const bool do_move = all_axes_homed() && !marlin_server::printer_idle();

    if (do_move) {
        do_blocking_move_to_xy(X_NOZZLE_PARK_POINT, Y_NOZZLE_PARK_POINT);
    }

    while (buddy::emergency_stop().do_stop) {
        idle(true, true);
    }

    if (do_move) {
        do_blocking_move_to_xy(old.x, old.y);
    }

    marlin_server::clear_warning(WarningType::DoorOpen);
}
