/// @file
#pragma once

#include <optional>

#include "base_hotend.hpp"
#include <tool/tool/standard_fff_physical_tool.hpp>

/// Represents a hotend that is managed by the INDX head
class IndxHotend final : public BaseHotend {
    friend class PrusaToolChanger; // For access to start_heating / stop_heating
    friend class PrusaToolChangerUtils;

public:
    /// !!! Careful, the config pointer is stored, so make sure the config is persistent!
    explicit IndxHotend(PhysicalToolIndex tool, const Config *config)
        : BaseHotend(tool, config) {
        is_thermally_managed_ = false; // INDX starts inactive
        nozzle_temp_ = std::nullopt; // parked sentinel until first pickup; kept in sync with stop_heating()
    }

    static StandardFFFPhysicalTool<IndxHotend> &indx_tool(PhysicalToolIndex tool);

    /// Asserts the toolchanger thermal-management invariant: no tool other than
    /// `expected_managed` may currently be thermally managed. Pass NoTool to assert
    /// that no tool is managed at all.
    static void assert_thermally_managed_invariant(std::variant<PhysicalToolIndex, NoTool> expected_managed);

    /// Resolves an in-flight recheck; call every cycle() (also while no tool is managed).
    static void process_pending_thermal_runaway();

    /// Heater selftest trip interception (mirrors the fan controllers' selftest mode): while
    /// engaged, thermal-protection trips (thermal model, heater watch, thermal runaway) are
    /// recorded instead of raising a fatal error. The heater is still switched off immediately
    /// on a trip. !!! Marlin thread only.
    static void enter_heater_selftest_mode();
    static void exit_heater_selftest_mode();

    /// @returns the error code of the first trip since enter_heater_selftest_mode(), if any
    static std::optional<ErrCode> heater_selftest_trip();

protected:
    virtual void manage() override;

    /// Re-verifies nozzle presence before raising; a fallen nozzle looks like a runaway.
    void invoke_thermal_runaway(ErrCode error_code) override;

    /// Pushes the current target to the puppy and resets the hotend temp compensator.
    /// Engages other heating side effects common to all hotend implementations.
    /// changes (via BaseHotend::set_nozzle_target_temp) and on pickup (via start_heating).
    void handle_nozzle_target_change() override;

private:
    /// Fires heating side effects (safety_timer wake, power_on, watch reset, puppy push,
    /// temp model reset) and arms protection state machines.
    /// !!! Toolchanger-only: use at pickup, when transitioning into active heating. !!!
    void start_heating();

    /// Stop heating physically. Logical target is preserved so a future pickup can resume.
    /// !!! Toolchanger-only: use at park, open_head, recovery — not as a way to set target=0. !!!
    void stop_heating();

private:
    /// Last checked value of indx_head.get_reset_counter(), for tracking indx head resets
    uint32_t last_head_reset_count_ = 0;
};
