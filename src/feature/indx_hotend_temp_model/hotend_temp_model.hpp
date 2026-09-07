/// @file
#pragma once

#include <cstdint>
#include <variant>

#include <tool_index.hpp>

#include "indx_hotend_temp_compensation.hpp"
#include "indx_hotend_thermal_model.hpp"
#include "indx_filament_params.hpp"

#include <utils/timing/rate_limiter.hpp>

// The thermometer on the head is basically not close enough to the melt zone
// and there is a significant difference between the measured temperature and what temp the filament is actually on.
// To compensate for this, we calculate a rudimentary filament model on the motherboard
// and send a compensation parameter over the modbus to the head.
// BFW-8630
//
// Also, we run a thermal model of the hotend to detect thermal runaway
// BFW-8702
namespace buddy {

/// Wrapper over the platform-invariant compensator that wraps indx_hotend_temp_compensation::HotendTempCompensator
/// Works directly on the INDX-head, going around the IndxHotend. This makes sense to be a singleton.
/// !!! To be accessed from marlin thread only
class INDXHotendTempModel {

public:
    /// Binds the model to @p tool, or NoTool on park. Resets state for lazy re-init on the next
    /// step() and clears the head temp compensation. Call on pickup (tool) and park (NoTool).
    void set_tool(std::variant<VirtualToolIndex, NoTool> tool);

    /// To be called in regular intervals from marlin thread while the hotend is thermally managed.
    /// @returns true when a thermal runaway is detected; the caller is expected to raise the error.
    [[nodiscard]] bool step();

    /// Forces re-initialization on the next step(). Call when the indx head resets.
    void reset();

    /// To be called when filament parameters might have changed
    void update_filament_params();

private:
    /// The virtual tool being modeled this session, or NoTool when not thermally managed
    std::variant<VirtualToolIndex, NoTool> managed_tool_ = NoTool {};
    indx::FilamentParameters filament_params_;
    ::indx_hotend_temp_compensation::HotendTempCompensator compensator_;
    indx::HotendThermalModel thermal_model_;

    RateLimiter<uint32_t> step_limiter_ms_ { 100 };

    // These don't need to be initialized, they get initialized before is_initialized_ goes to true
    int32_t last_e_steps_;
    float retracted_distance_mm_;
    FilamentType last_filament_;

    bool is_initialized_ : 1 = false;
    bool filament_data_update_pending_ : 1 = false;
};

/// Singleton
/// !!! To be accessed from marlin thread only
INDXHotendTempModel &hotend_temp_model();

} // namespace buddy
