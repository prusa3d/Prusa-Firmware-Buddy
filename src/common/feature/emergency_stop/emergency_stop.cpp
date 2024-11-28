#include "emergency_stop.hpp"
#include <buddy/door_sensor.hpp>
#include <config_store/store_c_api.h>
#include <common/power_panic.hpp>
#include <module/stepper.h>
#include <marlin_server.hpp>

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

    int32_t current_z() {
        return stepper.position_from_startup(Z_AXIS);
    }

    // Try to do some desperate measures to stop moving in Z (currently implemented as power panic).
    void invoke_emergency() {
        log_info(EmergencyStop, "Emergency stop");
        // Do a "synthetic" power panic. Should stop _right now_ and reboot, then we'll deal with the consequences.
        if (!power_panic::ac_fault_triggered) {
            // this is normally supposed to be called from ISR, but since disables IRQ so it works fine even outside of ISR
            power_panic::ac_fault_isr();
        }
    }
} // namespace

void EmergencyStop::emergency_start() {
    log_info(EmergencyStop, "Emergency start");
    const auto steps = get_steps_per_unit_z();
    allowed_steps = allowed_mm * steps;
    extra_emergency_steps = extra_emergency_mm * steps;
    start_z = current_z();
}

void EmergencyStop::emergency_over() {
    log_info(EmergencyStop, "Emergency over");
    start_z = no_emergency;
}

void EmergencyStop::maybe_block() {
    if (!in_emergency()) {
        return;
    }

    switch (maybe_block_state) {

    case MaybeBlockState::not_running:
        break;

    case MaybeBlockState::running:
        // We should never get maybe_block called while executing maybe_block (maybe_block -> idle -> maybe_block).
        // That would mean that we are trying to do moves during emergency blocking that have not beed issued by this function.
        bsod("Nested EmergencyStop::maybe_block");

    case MaybeBlockState::executing_move:
        // We're currently parking/unparking from within this function - prevent nesting and exit.
        return;
    }

    AutoRestore _ar(maybe_block_state, MaybeBlockState::running);
    marlin_server::set_warning(WarningType::DoorOpen, PhasesWarning::DoorOpen);
    ScopeGuard warning_guard = [] {
        marlin_server::clear_warning(WarningType::DoorOpen);
    };

    // Don't park:
    // * If parking would mean we have to home first (which'll look bad, but also move in Z, which'd do Bad Things).
    // * If we are not actually printing.
    const bool do_move = all_axes_homed() && !marlin_server::printer_idle();
    const auto old_pos = current_position;
    if (do_move) {
        AutoRestore _ar(maybe_block_state, MaybeBlockState::executing_move);
        do_blocking_move_to_xy(X_NOZZLE_PARK_POINT, Y_NOZZLE_PARK_POINT);
    }
    auto unpark = [this, old_pos] {
        AutoRestore _ar(maybe_block_state, MaybeBlockState::executing_move);
        do_blocking_move_to_xy(old_pos.x, old_pos.y);
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
        if (difference > allowed_steps) {
            if (difference > extra_emergency_steps) {
                // Didn't work the first time around? What??
                bsod("Emergency stop failed, last-resort stop");
            } else {
                invoke_emergency();
            }
        }
    }
}

void EmergencyStop::step() {
    const bool is_door_closed = door_sensor().state() == DoorSensor::State::door_closed;
    const bool is_emergency_stop_enabled = config_store().emergency_stop_enable.get();
    const bool want_emergency = !is_door_closed && is_emergency_stop_enabled;
    if (want_emergency && !in_emergency()) {
        emergency_start();
    } else if (!want_emergency && in_emergency()) {
        emergency_over();
    }
}

EmergencyStop &emergency_stop() {
    static EmergencyStop instance;
    return instance;
}

} // namespace buddy
