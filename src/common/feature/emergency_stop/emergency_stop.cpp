#include "emergency_stop.hpp"
#include <buddy/door_sensor.hpp>
#include <config_store/store_c_api.h>
#include <common/power_panic.hpp>
#include <module/stepper.h>
#include <marlin_server.hpp>
#include <Configuration.h>

#include <logging/log.hpp>
#include <RAII.hpp>
#include <scope_guard.hpp>

LOG_COMPONENT_DEF(EmergencyStop, logging::Severity::debug);

namespace buddy {

namespace {
    // TODO: Tune? 2mm is _probably_ OK (it won't squish a finger too much even
    // if it was a tight fit before), but someone needs to confirm.

    // We allow this distance to be traveled before doing any panicky things -
    // we'll just schedule the stop, wait for the moves to get through Planner
    // and then park. Only if we travel in Z by more than this, we start doing
    // a bit more.
    constexpr float allowed_mm = 2.0;

    // If we travel even more before any of the above measures had a chance to
    // stop it, we do a BSOD as a last resort.
    constexpr float extra_emergency_mm = 4.0;

    // Don't park below this position.
    constexpr float min_park_z = 0.6;

#ifdef INCH_MODE_SUPPORT
    #error "Implement unit conversion here."
#endif

    int32_t current_z() {
        return stepper.position_from_startup(Z_AXIS);
    }
} // namespace

// Try to do some desperate measures to stop moving in Z (currently implemented as power panic or quick_stop).
void EmergencyStop::invoke_emergency() {
    log_info(EmergencyStop, "Emergency stop");
    emergency_invoked = true;

    const bool in_quickstoppable_state = marlin_server::printer_idle() || marlin_server::aborting_or_aborted() || marlin_server::finishing_or_finished();
    if (in_quickstoppable_state || !marlin_server::all_axes_homed()) {
        log_info(EmergencyStop, "Quickstop");
        planner.quick_stop();
        while (PreciseStepping::stopping()) {
            PreciseStepping::loop();
        }
        planner.clear_block_buffer();
        planner.resume_queuing();
        // We've lost the homing by the quick-stop
        //
        // In case we are in print, we are here because we are still homing /
        // aren't homed yet, so that's fine to keep (and to keep partial
        // homing, because until then, we move slowly and in straight lines
        // anyway).
        if (in_quickstoppable_state) {
            set_all_unhomed();
        }

    } else if (!power_panic::ac_fault_triggered) {
        log_info(EmergencyStop, "PP");
        // Do a "synthetic" power panic. Should stop _right now_ and reboot, then we'll deal with the consequences.
        // Do not beep - BFW-6472
        power_panic::should_beep = false;
        buddy::hw::acFault.triggerIT();

    } else {
        log_info(EmergencyStop, "Out of options");
    }
}

void EmergencyStop::maybe_block() {
    // The default step might not be called often/fast enough - we want to check we're having up-to-date data when deciding whether we should block
    step();

    if (!in_emergency()) {
        return;
    }

    // Prevent maybe_block nested calls
    if (maybe_block_running) {
        return;
    }

    // If a power panic happened (either caused by us or by a real one), we do
    // _not_ want to block it.
    if (power_panic::ac_fault_triggered) {
        return;
    }

    marlin_server::set_warning(WarningType::DoorOpen);
    maybe_block_running = true;
    allow_planning_movements = false;

    ScopeGuard _sg = [this] {
        maybe_block_running = false;
        allow_planning_movements = true;
        marlin_server::clear_warning(WarningType::DoorOpen);
    };

    // If the emergency ended before we've finished the planned moves, there's no point in parking the head -> exit now
    planner.synchronize();
    if (!in_emergency()) {
        return;
    }

    // Don't park:
    // * If parking would mean we have to home first (which'll look bad, but also move in Z, which'd do Bad Things).
    // * If we are not actually printing.
    // * If we are in/around pause (it was behaving a bit confused).
    const bool do_move = all_axes_homed() && !marlin_server::printer_idle() && !marlin_server::printer_paused_extended();
    // We are manipulating the moves "under the hands" of other stuff, and "in
    // the middle" of other stuff.
    //
    // Make sure to take what's current in _planner_ (because motion adjust
    // current_position only after the whole, possibly multi-segmented move
    // gets submitted, which we possibly interrupt). Also, make sure we return
    // to the original position in all places (because we are not 100% sure
    // which positions are with or without MBL).
    const auto old_pos = planner.get_machine_position_mm();
    const auto old_pos_motion = current_position;
    if (do_move) {
        // Make sure to not park too low. As the do_blocking_move_to doesn't
        // consider MBL (and we may not have that part mapped anyway), we could
        // scratch the bed.
        const auto park_z = std::max(old_pos.z, min_park_z);
        AutoRestore _ar(allow_planning_movements, true);
        // All the do-move things expect the current position to be up to date.
        // It is _not_ (because we might have interrupted another move in the
        // middle). This is the best estimation we have for it (might be wrong
        // by MBL :-( ). Should we un-apply it somehow?
        current_position = old_pos;
        do_blocking_move_to(X_NOZZLE_PARK_POINT, Y_NOZZLE_PARK_POINT, park_z, feedRate_t(NOZZLE_PARK_XY_FEEDRATE));
    }
    auto unpark = [this, old_pos, old_pos_motion] {
        AutoRestore _ar(allow_planning_movements, true);
        do_blocking_move_to(old_pos.x, old_pos.y, old_pos.z, feedRate_t(NOZZLE_PARK_XY_FEEDRATE));
        current_position = old_pos_motion;
    };
    ScopeGuard unpark_guard(std::move(unpark), do_move);

    // Wait for the emergency to be over.
    //
    // If the power panic started the draining, we shall quit from inside of
    // the planner as fast as possible.
    while (in_emergency() && !planner.draining() && !PreciseStepping::stopping() && !power_panic::ac_fault_triggered) {
        idle(true, true);
    }

    // Trigger the scope guards: unpark, clear the warning
}

void EmergencyStop::check_z_limits() {
    const int32_t emergency_start_z = start_z.load();
    if (emergency_start_z != no_emergency) {
        const int32_t difference = std::abs(emergency_start_z - current_z());
        if (difference > extra_emergency_steps) {
            // Didn't work the first time around? What??
            // (see check_z_limits_soft)
            bsod("Emergency stop failed, last-resort stop");
        }
    }
}

void EmergencyStop::check_z_limits_soft() {
    const int32_t emergency_start_z = start_z.load();
    if (emergency_start_z != no_emergency) {
        const int32_t difference = std::abs(emergency_start_z - current_z());
        if (difference > allowed_steps && !emergency_invoked) {
            invoke_emergency();
        }
    }
}

void EmergencyStop::step() {
    const bool is_door_closed = door_sensor().state() == DoorSensor::State::door_closed;
    const bool is_emergency_stop_enabled = config_store().emergency_stop_enable.get();
    const bool want_emergency = !is_door_closed && is_emergency_stop_enabled;

    if (want_emergency && !in_emergency()) {
        log_info(EmergencyStop, "Emergency start");
        const auto steps = get_steps_per_unit_z();
        allowed_steps = allowed_mm * steps;
        extra_emergency_steps = extra_emergency_mm * steps;
        start_z = current_z();
    } else if (!want_emergency && in_emergency()) {
        log_info(EmergencyStop, "Emergency over");
        start_z = no_emergency;
        emergency_invoked = false;
    }

    check_z_limits_soft();
}

EmergencyStop &emergency_stop() {
    static EmergencyStop instance;
    return instance;
}

void EmergencyStop::assert_can_plan_movement() {
    if (!allow_planning_movements) {
        bsod("Unexpected movement request");
    }
}
} // namespace buddy
