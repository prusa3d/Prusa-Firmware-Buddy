#include "emergency_stop.hpp"
#include <buddy/door_sensor.hpp>
#include <common/mapi/parking.hpp>
#include <config_store/store_c_api.h>
#include <common/power_panic.hpp>
#include <module/motion.h>
#include <module/planner.h>
#include <module/stepper.h>
#include <module/endstops.h>
#include <marlin_server.hpp>
#include <Configuration.h>

#include <logging/log.hpp>
#include <raii/auto_restore.hpp>
#include <raii/scope_guard.hpp>
#include <bsod/bsod.h>
#include <option/has_power_panic.h>

LOG_COMPONENT_DEF(EmergencyStop, logging::Severity::debug);

static_assert(std::to_underlying(WarningType::DoorOpen) == 0, "Door open should have the highest priority for GUI warning to show properly");

namespace buddy {

namespace {
    // TODO: Tune? 2mm is _probably_ OK (it won't squish a finger too much even
    // if it was a tight fit before), but someone needs to confirm.

    // We allow this distance to be traveled before doing any panicky things -
    // we'll just schedule the stop, wait for the moves to get through Planner
    // and then park. Only if we travel in Z by more than this, we start doing
    // a bit more.
    constexpr float allowed_mm = 2.0f;

    // ISR power-panic-escalation threshold: the loop-driven stop clearly didn't
    // act in time. Kept below extra_emergency_mm with room for the power panic's
    // own Z-lift (POWER_PANIC_Z_LIFT_CYCLES, ~0.64 mm) to settle first.
    constexpr float escalate_mm = 3.0f;

    // If we travel even more before any of the above measures had a chance to
    // stop it, we do a BSOD as a last resort.
    constexpr float extra_emergency_mm = 4.0f;

    static_assert(allowed_mm < escalate_mm && escalate_mm < extra_emergency_mm);

    // Don't park below this position.
    constexpr float min_park_z = 0.6f;

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
        planner.quick_stop_and_resume();
        // We've lost the homing by the quick-stop
        //
        // In case we are in print, we are here because we are still homing /
        // aren't homed yet, so that's fine to keep (and to keep partial
        // homing, because until then, we move slowly and in straight lines
        // anyway).
        if (in_quickstoppable_state) {
            set_all_unhomed();
        }

    } else if (endstops.is_z_probe_enabled()) {
        // We can safely do this, because this is what Planner::endstop_triggered(Z_AXIS) does internally
        // Except we don't trigger the endstop, so the probing will be reported as failed
        PreciseStepping::quick_stop();
    }
#if HAS_POWER_PANIC()
    else if (!power_panic::ac_fault_triggered) {
        log_info(EmergencyStop, "PP");
        // Do a "synthetic" power panic. Should stop _right now_ and reboot, then we'll deal with the consequences.
        // Do not beep - BFW-6472
        power_panic::should_beep = false;
        buddy::hw::acFault.triggerIT();

    }
#endif
    else {
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

    if (endstops.is_z_probe_enabled()) {
        // Prevent getting stuck on planner.synchronize() - quick_stop makes the planner busy again
        // So issue it only if there are any moves to be interrupted
        if (planner.busy()) {
            // Don't wait for the probing to finish, interrupt it immediately
            // We can safely do this, because this is what Planner::endstop_triggered(Z_AXIS) does internally
            // Except we don't trigger the endstop, so the probing will be reported as failed
            PreciseStepping::quick_stop();
        }

        // Don't do anything else, let the quick_stop apply and get us out of the probing code
        // We cannot safely block here, because:
        // * We cannot park the nozzle - it would not play well with the quick_stop mechanism the endstops are using
        //   (and that would be enabled during the whole parking business)
        // * If whe block without parking, we might block at the moment when the nozzle is touching the plate. We don't want a hole in our plate.
        // So it's better to interrupt the Z probe move and park right after we get outside of the probing code.
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
    const MachinePosXYZE pos = planner.get_machine_position_mm();
    const bool do_move =
        // If parking would mean we have to home first (which'll look bad, but also move in Z, which'd do Bad Things).
        all_axes_homed()

        //  If we are not actually printing.
        && !marlin_server::printer_idle()

        // If we are in/around pause (it was behaving a bit confused).
        && !marlin_server::printer_paused_extended()

        // Don't park if we are below the print high-water mark - parking could
        // mean an XY traverse through an already-printed object during sequential
        // printing (with luck we could avoid it in XY, but above is fine and
        // that's when we allow it).
        && (current_position.z >= planner.max_printed_z)

        // If we are outside the print region, where parking could disrupt a toolchange or cross the wastebin boundary.
        && is_xy_in_print_region(pos.xy());

    // We are manipulating the moves "under the hands" of other stuff, and "in
    // the middle" of other stuff.
    //
    // Make sure to take what's current in _planner_ (because motion adjust
    // current_position only after the whole, possibly multi-segmented move
    // gets submitted, which we possibly interrupt). Also, make sure we return
    // to the original position in all places (because we are not 100% sure
    // which positions are with or without MBL).
    const auto old_pos_motion = current_position;
    const auto old_destination = destination;
    if (do_move) {
        AutoRestore _ar(allow_planning_movements, true);
        auto park_position = mapi::get_parking_position(mapi::ParkPosition::park);
        // Stay at the current Z instead of the standard lift above the print,
        // just don't park too low so we don't scratch the bed.
        park_position.z = mapi::ParkingPosition::AtLeast { .absolute = min_park_z };
        mapi::park(park_position);
    }
    auto unpark = [this, old_pos_motion, old_destination] {
        AutoRestore _ar(allow_planning_movements, true);
        mapi::park(mapi::ParkingPosition { old_pos_motion.x, old_pos_motion.y, old_pos_motion.z });
        current_position = old_pos_motion;
        destination = old_destination;
    };
    ScopeGuard unpark_guard(std::move(unpark), do_move);

    // Wait for the emergency to be over.
    //
    // If the power panic started the draining, we shall quit from inside of
    // the planner as fast as possible.
    while (in_emergency() && !planner.draining() && !PreciseStepping::stopping() && !power_panic::ac_fault_triggered) {
        idle(true);
    }

    // Trigger the scope guards: unpark, clear the warning
}

void EmergencyStop::check_z_limits() {
    const int32_t emergency_start_z = start_z.load();
    if (emergency_start_z == no_emergency) {
        return;
    }
    const int32_t difference = std::abs(emergency_start_z - current_z());

#if HAS_POWER_PANIC()
    // Loop-driven stop didn't act in time (stalled loop). Escalate from the ISR
    // - unlike the BSOD this can still recover the print.
    if (difference > escalate_steps && !power_panic::ac_fault_triggered) {
        power_panic::should_beep = false; // BFW-6472
        buddy::hw::acFault.triggerIT();
    } else
#endif
        if (difference > extra_emergency_steps) {
        bsod("Emergency stop failed, last-resort stop");
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
        allowed_steps = static_cast<int32_t>(allowed_mm * steps);
        escalate_steps = static_cast<int32_t>(escalate_mm * steps);
        extra_emergency_steps = static_cast<int32_t>(extra_emergency_mm * steps);
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
