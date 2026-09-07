/// @file
#include "indx_hotend.hpp"

#include <optional>

#include <bsod.h>
#include <wiring_time.h>
#include <core/millis_t.h>
#include <logging/log.hpp>
#include <puppies/INDX.hpp>
#include <common/aggregate_arity.hpp>
#include <feature/indx_hotend_temp_model/hotend_temp_model.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(Marlin);

namespace {
struct PendingThermalRunaway {
    ErrCode error_code;
    uint32_t started_ms;
};

/// In-flight nozzle presence recheck; marlin task only, so no synchronization is needed.
std::optional<PendingThermalRunaway> pending_thermal_runaway;

/// Fail-safe timeout while presence is unknown; with the heater off the head re-measures fast.
constexpr uint32_t presence_data_timeout_ms = 1000;

/// Fail-safe timeout once the nozzle reads absent, before raising instead of waiting for the
/// toolchanger. Clears ~2 in-print check periods (PRINT_NOZZLE_CHECK_PERIOD_MS = 5 s); heater is off.
constexpr uint32_t recovery_takeover_timeout_ms = 12000;

/// Heater selftest trip interception; marlin task only, so no synchronization is needed.
bool selftest_trip_interception_active = false;
std::optional<ErrCode> selftest_intercepted_trip;
} // namespace

void IndxHotend::handle_nozzle_target_change() {
    BaseHotend::handle_nozzle_target_change();

    // Keep the heater off while a recheck is pending (don't let M104/M109 re-enable it).
    if (pending_thermal_runaway) {
        buddy::puppies::indx.set_hotend_target_temp(0);
    } else {
        buddy::puppies::indx.set_hotend_target_temp(nozzle_target_temp());
    }

    // Temp change indicates possible filament parameters change, recompute
    buddy::hotend_temp_model().update_filament_params();
}

void IndxHotend::start_heating() {
    assert_thermally_managed_invariant(NoTool {});
    is_thermally_managed_ = true;
    // Bind the thermal model to this tool's managed session.
    buddy::hotend_temp_model().set_tool(tool_.currently_selected_virtual_tool());
    // Sync now (manage() didn't run while parked) so the first manage() won't re-fire on a park-time reset.
    last_head_reset_count_ = buddy::puppies::indx.get_reset_counter();
    handle_nozzle_target_change();
}

void IndxHotend::stop_heating() {
    // target doesn't change so the next heating can heat to original target
    buddy::puppies::indx.set_hotend_target_temp(0);
    // tool is still physically on head, but it is no longer thermally managed
    is_thermally_managed_ = false;

    nozzle_temp_ = std::nullopt;
    nozzle_heater_pwm_ = 0;

    // No longer thermally managed: unbind the thermal model, zeroing its head compensation.
    buddy::hotend_temp_model().set_tool(NoTool {});

    assert_thermally_managed_invariant(NoTool {});
}

void IndxHotend::assert_thermally_managed_invariant(std::variant<PhysicalToolIndex, NoTool> expected_managed) {
    for (auto t : PhysicalToolIndex::all()) {
        if (stdext::holds_value(expected_managed, t)) {
            continue;
        }
        if (IndxHotend::indx_tool(t).hotend().is_thermally_managed()) {
            bsod_unreachable();
        }
    }
}

void IndxHotend::enter_heater_selftest_mode() {
    selftest_intercepted_trip.reset();
    selftest_trip_interception_active = true;
}

void IndxHotend::exit_heater_selftest_mode() {
    selftest_trip_interception_active = false;
    selftest_intercepted_trip.reset();
}

std::optional<ErrCode> IndxHotend::heater_selftest_trip() {
    return selftest_intercepted_trip;
}

void IndxHotend::invoke_thermal_runaway(ErrCode error_code) {
    if (selftest_trip_interception_active) {
        // Heater selftest in progress: the trip is the test's verdict, not a fatal error.
        // The heater goes off immediately either way; the selftest handles the rest.
        buddy::puppies::indx.set_hotend_target_temp(0);
        if (!selftest_intercepted_trip) {
            selftest_intercepted_trip = error_code;
        }
        return;
    }

    if (pending_thermal_runaway) {
        // A recheck is already in flight; the first error code wins.
        return;
    }

    // Stop heating but stay thermally managed: process_pending() reads "nothing managed" as the
    // toolchanger having taken over. Not stop_heating() - that transition is the toolchanger's job.
    buddy::puppies::indx.set_hotend_target_temp(0);

    const auto nozzle_present = buddy::puppies::indx.get_nozzle_present();
    const bool definitively_absent = nozzle_present.has_value() && !*nozzle_present;
    if (!definitively_absent) {
        // Force a fresh measurement (a fallen nozzle looks like a runaway); definitive absent is kept.
        buddy::puppies::indx.invalidate_nozzle_data();
    }

    pending_thermal_runaway = PendingThermalRunaway { .error_code = error_code, .started_ms = millis() };
    log_warning(Marlin, "Thermal protection tripped, rechecking nozzle presence");
}

void IndxHotend::process_pending_thermal_runaway() {
    if (!pending_thermal_runaway) {
        return;
    }

    bool any_tool_managed = false;
    for (auto tool : PhysicalToolIndex::all()) {
        any_tool_managed |= Hotend::for_tool(tool).is_thermally_managed();
    }
    if (!any_tool_managed) {
        // stop_heating() ran in the meantime (park / recovery / abort), so the trigger is moot.
        log_info(Marlin, "Thermal protection recheck dropped, heating already stopped");
        pending_thermal_runaway.reset();
        return;
    }

    const auto nozzle_present = buddy::puppies::indx.get_nozzle_present();
    if (nozzle_present.has_value() && *nozzle_present) {
        // Nozzle is in the head - this is a genuine thermal problem.
        fatal_error(pending_thermal_runaway->error_code);
    }

    // Absent or still unknown: normally the toolchanger recovery takes over (dropped above once
    // nothing is managed), but it can be blocked (paused print, open FSM, tool_lost reheat), so raise
    // on a deadline instead of hanging. Absent gets the longer budget to let the in-print check run.
    const bool nozzle_absent = nozzle_present.has_value() && !*nozzle_present;
    const uint32_t timeout_ms = nozzle_absent ? recovery_takeover_timeout_ms : presence_data_timeout_ms;
    if (!PENDING(millis(), pending_thermal_runaway->started_ms + timeout_ms)) {
        log_warning(Marlin, "Nozzle presence recheck timed out, raising thermal error");
        fatal_error(pending_thermal_runaway->error_code);
    }
}

void IndxHotend::manage() {
    debug_assert(is_thermally_managed());

    // self-paced internally, so calling it each manage() tick is fine.
    if (buddy::hotend_temp_model().step()) {
        invoke_thermal_runaway(ErrCode::ERR_TEMPERATURE_HOTEND_THERMAL_RUNAWAY);
    }

    const auto reset_count = buddy::puppies::indx.get_reset_counter();
    const bool head_got_reset = (reset_count != last_head_reset_count_);
    last_head_reset_count_ = reset_count;

    nozzle_temp_ = buddy::puppies::indx.get_hotend_temp_compensated();
    nozzle_heater_pwm_ = buddy::puppies::indx.get_hotend_pwm_averaged();

    if (head_got_reset) {
        // Act as if nozzle target temp changed
        // This resets all the safety guards (heater watch, thermal runaway, ...)
        // to prevent them from semi-falsely triggering when the indx head reset (which caused a temporary heating dropout)
        handle_nozzle_target_change();
        // Re-init the thermal model so it doesn't run against the reset discontinuity
        buddy::hotend_temp_model().reset();
    }

    BaseHotend::manage();
}
