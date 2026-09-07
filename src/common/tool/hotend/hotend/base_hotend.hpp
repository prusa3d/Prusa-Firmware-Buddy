/// @file
#pragma once

#include <tool/hotend/hotend.hpp>
#include <module/temperature/thermal_runaway.hpp>
#include <module/temperature/heater_watch.hpp>
#include <error_codes.hpp>

/// Represents a base for all non-dummy hotends
class BaseHotend : public Hotend {

public:
    void filament_compatibility_report(FilamentCompatibilityReport &report, const FilamentCompatibilityReportGenerateArgs &args) const override;

    void set_nozzle_target_temp(TargetTemperature set) final override;

#if HAS_TEMP_HEATBREAK_CONTROL
    void set_heatbreak_target_temp(TargetTemperature set) override;
#endif

protected:
    /// Called by the thermal protection detectors instead of raising directly.
    /// Base raises immediately; IndxHotend overrides to re-verify nozzle presence first.
    virtual void invoke_thermal_runaway(ErrCode error_code);

    /// Re-arms protection state machines on heating-change events: nozzle target
    /// temperature change while managed, or transition into managed state.
    virtual void handle_nozzle_target_change();

    /// !!! Careful, the config pointer is stored, so make sure the config is persistent!
    explicit BaseHotend(PhysicalToolIndex tool, const Config *config);

    // !!! MUST be called after temps are set properly
    // Note: the = 0; is here to enforce overriding.
    // !!! The function is actually implemented and MUST be called from the overriding function.
    // Precondition: only called for thermally-managed hotends
    // Running this on a non-managed hotend would false-trigger min_temp_error (parked temp < mintemp with a stale target).
    virtual void manage() override = 0;

    void manage_temp_residency();

protected:
    const PhysicalToolIndex tool_;

#if ENABLED(THERMAL_PROTECTION_HOTENDS)
    ThermalRunaway thermal_runaway_;
#endif

#if WATCH_HOTENDS
    HeaterWatch heater_watch_;
#endif

    /// timestamp when temeperature reached target +-TEMP_WINDOW, 0 when outside this window
    /// note: 0 is valid timestamp, but if temperature reaches window at time 0, it will just be evaluated again little later, so it doesn't cause any bug
    uint32_t nozzle_temp_residency_start_ms_ = 0;
};
