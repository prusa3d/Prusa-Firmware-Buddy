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

#ifdef INCH_MODE_SUPPORT
    #error "Implement unit conversion here."
#endif

    int32_t current_z() {
        return stepper.position_from_startup(Z_AXIS);
    }
} // namespace

// Try to do some desperate measures to stop moving in Z (currently implemented as power panic).
void EmergencyStop::invoke_emergency() {
    log_info(EmergencyStop, "Emergency stop");
    // TODO: We would really like to include the last parking moves after a
    // finished print too, because power panic is no longer ready at that
    // point.
    if (marlin_server::printer_idle()) {
        planner.quick_stop();
        while (PreciseStepping::stopping()) {
            PreciseStepping::loop();
        }
        planner.clear_block_buffer();
        planner.resume_queuing();
        // We've lost the homing by the quick-stop
        set_all_unhomed();
    } else if (!power_panic::ac_fault_triggered) {
        // Do a "synthetic" power panic. Should stop _right now_ and reboot, then we'll deal with the consequences.

        // this is normally supposed to be called from ISR, but since disables IRQ so it works fine even outside of ISR
        power_panic::ac_fault_isr();
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

    marlin_server::set_warning(WarningType::DoorOpen, PhasesWarning::DoorOpen);
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
    const bool do_move = all_axes_homed() && !marlin_server::printer_idle();
    // We are manipulating the moves "under the hands" of other stuff, and "in
    // the middle" of other stuff.
    //
    // Make sure to take what's current in _planner_ (because motion adjust
    // current_position only after the whole, possibly multi-segmented move
    // gets submitted, which we possibly interrupt). Also, make sure we return
    // to the original position in all places (because we are not 100% sure
    // which positions are with or without MBL).
    const auto old_pos = planner.get_machine_position_mm();
    [[maybe_unused]] const auto old_pos_msteps = planner.get_position_msteps();
    const auto old_pos_motion = current_position;
    if (do_move) {
        AutoRestore _ar(allow_planning_movements, true);
        do_blocking_move_to(X_NOZZLE_PARK_POINT, Y_NOZZLE_PARK_POINT, old_pos.z);
    }
    auto unpark = [this, old_pos, old_pos_motion, old_pos_msteps] {
        AutoRestore _ar(allow_planning_movements, true);
        do_blocking_move_to(old_pos.x, old_pos.y, old_pos.z);
        current_position = old_pos_motion;
        // Note: The extruder can still endup in a different position because
        // of pressure advance (probably); eliminate false assert on these, we
        // worry about X, Y, Z here.
        assert(planner.position_float.x == old_pos.x);
        assert(planner.position_float.y == old_pos.y);
        assert(planner.position_float.z == old_pos.z);
        assert(planner.position.x == old_pos_msteps.x);
        assert(planner.position.y == old_pos_msteps.y);
        assert(planner.position.z == old_pos_msteps.z);
    };
    ScopeGuard unpark_guard(std::move(unpark), do_move);

    // Wait for the emergency to be over
    while (in_emergency()) {
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
        if (difference > allowed_steps) {
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
