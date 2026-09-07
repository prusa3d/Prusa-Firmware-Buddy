/// @file
#include "base_hotend.hpp"

#include <bsod.h>
#include <module/temperature.h>
#include <filament.hpp>
#include <feature/compatibility_checks/filament_compatibility.hpp>

#include <option/board_is_master_board.h>
#if BOARD_IS_MASTER_BOARD()
    #include <marlin_server.hpp>
    #include <feature/safety_timer/safety_timer.hpp>
#endif

#if WATCH_HOTENDS
static constexpr HeaterWatch::Config heater_watch_config {
    .temp_increase = WATCH_TEMP_INCREASE,
    .period_s = WATCH_TEMP_PERIOD,
    .min_temp_diff = WATCH_TEMP_INCREASE + TEMP_HYSTERESIS + 1,
};
#endif

BaseHotend::BaseHotend(PhysicalToolIndex tool, const Config *config)
    : Hotend(*config)
    , tool_(tool)
#if WATCH_HOTENDS
    , heater_watch_(heater_watch_config)
#endif
{
}

void BaseHotend::filament_compatibility_report(FilamentCompatibilityReport &report, const FilamentCompatibilityReportGenerateArgs &args) const {
    if (base_config_.max_nozzle_temp < args.filament.nozzle_temperature) {
        report.failed_tool_checks.set(buddy::filament_compatibility::ToolCheck::tool_max_temp);
    }
}

void BaseHotend::set_nozzle_target_temp(TargetTemperature set) {
#if BOARD_IS_MASTER_BOARD()
    // We cannot overwrite target temps while the safety_timer is active, deactivate it first
    buddy::safety_timer().reset_restore_nonblocking();
#endif

    const int16_t new_temp = std::min<int16_t>(set, base_config_.max_nozzle_temp - HEATER_MAXTEMP_SAFETY_MARGIN);

    if (nozzle_target_temp_ != new_temp) {
        nozzle_target_temp_ = new_temp;

#if BOARD_IS_MASTER_BOARD()
        marlin_server::call_manually::set_temp_to_display(new_temp, tool_);
#endif

        if (is_thermally_managed()) {
            handle_nozzle_target_change();
        }
    }
}

void BaseHotend::handle_nozzle_target_change() {
    nozzle_temp_residency_start_ms_ = 0;
    nozzle_temp_reached_ = false;
#if ENABLED(AUTO_POWER_CONTROL)
    if (nozzle_target_temp_ > 0) {
        powerManager.power_on();
    }
#endif
#if WATCH_HOTENDS
    heater_watch_.arm(nozzle_target_temp_);
#endif
#if ENABLED(THERMAL_PROTECTION_HOTENDS)
    thermal_runaway_.reset(nozzle_target_temp_);
#endif
}

#if HAS_TEMP_HEATBREAK_CONTROL
void BaseHotend::set_heatbreak_target_temp(TargetTemperature set) {
    #ifdef HEATBREAK_MAXTEMP
    set = std::min<TargetTemperature>(set, HEATBREAK_MAXTEMP);
    #endif

    heatbreak_target_temp_ = set;
}
#endif

void BaseHotend::invoke_thermal_runaway(ErrCode error_code) {
    fatal_error(error_code);
}

void BaseHotend::manage() {
    // Note: Checks in BaseHotend means that we're checking them twice on remote hotends if the remote hotend also uses this API (once on the master board, once on the remote tool board)
    // But better safe than sorry
    const OptionalTemperature maybe_nozzle_temp = nozzle_temp();

    if (!maybe_nozzle_temp.has_value()) {
        return;
    }

    const float curr_nozzle_temp = maybe_nozzle_temp.value();

    if (curr_nozzle_temp > base_config_.max_nozzle_temp) {
        thermalManager.max_temp_error((heater_ind_t)tool_.to_raw());
    }

    if ((nozzle_target_temp() > 0) && (curr_nozzle_temp < base_config_.min_nozzle_temp)) {
        thermalManager.min_temp_error((heater_ind_t)tool_.to_raw());
    }

    manage_temp_residency();

#if ENABLED(THERMAL_PROTECTION_HOTENDS)
    if (thermal_runaway_.step(curr_nozzle_temp, nozzle_target_temp(), THERMAL_PROTECTION_PERIOD, THERMAL_PROTECTION_HYSTERESIS)) {
        invoke_thermal_runaway(ErrCode::ERR_TEMPERATURE_HOTEND_THERMAL_RUNAWAY);
    }
#endif

#if WATCH_HOTENDS
    if (heater_watch_.update(curr_nozzle_temp)) {
        invoke_thermal_runaway(ErrCode::ERR_TEMPERATURE_HOTEND_PREHEAT_ERROR);
    }
#endif
}

void BaseHotend::manage_temp_residency() {
    const auto now = millis();
    const auto temp_diff = std::abs(nozzle_target_temp() - nozzle_temp().value());

    if (!nozzle_temp_residency_start_ms_ && temp_diff < TEMP_WINDOW) {
        nozzle_temp_residency_start_ms_ = now;

    } else if (temp_diff > TEMP_HYSTERESIS) {
        nozzle_temp_residency_start_ms_ = 0;
    }

    nozzle_temp_reached_ = //
        (nozzle_target_temp() <= 0) //
        || ( //
            nozzle_temp_residency_start_ms_ //
            && !PENDING(now, nozzle_temp_residency_start_ms_ + (TEMP_RESIDENCY_TIME)*1000UL) //
        );
}
