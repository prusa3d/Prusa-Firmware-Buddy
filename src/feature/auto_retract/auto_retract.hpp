/// @file
#pragma once

#include "tool_index.hpp"
#include <optional>
#include <bitset>
#include <inplace_function.hpp>
#include <option/has_indx.h>
#include <option/has_wastebin.h>
#include <option/has_nextruder.h>

namespace buddy {

namespace auto_retract_detail {
    using ProgressCallback = stdext::inplace_function<void(float progress_0_100)>;

    struct RetractFromNozzleParams {
        /// Callback for reporting progress of the retraction, called with values from 0 to 100. Optional.
        ProgressCallback progress_callback = nullptr;
#if HAS_WASTEBIN()
        /// Whether to park over the wastebin for the ramming sequence
        bool park_over_wastebin = true;
#endif
    };
} // namespace auto_retract_detail

/// Class for managing automatic retraction after print or load, so that the printer keeps the nozzle empty for MBL and non-printing to prevent oozing.
/// Only to be managed from the marlin thread
class AutoRetract {
    friend AutoRetract &auto_retract();

public:
    using ToolVariant = std::variant<PhysicalToolIndex, NoTool>;

#if HAS_NEXTRUDER()
    /// Minimum retract distance for the filament to be considered auto-retracted. Auto-retracted filaments can be unloaded without heating.
    static constexpr float full_retract_distance = 20.f; // mm
    static constexpr bool supports_cold_unload = true;

#elif HAS_INDX_HEAD()
    /// Retraction distance for a standard auto retract sequence
    static constexpr float full_retract_distance = 8.f; // mm

    /// In parked tools, the only thing holding the filament is the nozzle
    /// Thus we cannot auto-retract to the level that would allow cold unload
    static constexpr bool supports_cold_unload = false;
#else
    #error
#endif

    /// @returns whether the specified \param tool is retracted (some amount > 0.0f) and is a known value -> will deretract on positive Z move
    bool will_deretract(ToolVariant tool = PhysicalToolIndex::currently_selected()) const;

    /// @returns true if the filament retracted from the \param tool's nozzle for at least full_retract_distance
    bool is_fully_retracted(ToolVariant tool = PhysicalToolIndex::currently_selected()) const;

    /// @returns true if the filament is retracted enough for a cold unload (unloading without heating up the nozzle)
    bool can_cold_unload(PhysicalToolIndex physical_tool) const;

    /// How much is the filament retracted from the nozzle (mm), std::nullopt if retracted distance not a known value
    std::optional<float> retracted_distance(ToolVariant tool = PhysicalToolIndex::currently_selected()) const;

    /// If !is_fully_retracted(), executes the retraction process and saves retracted distance
    void maybe_retract_from_nozzle(const auto_retract_detail::RetractFromNozzleParams &params = {});

    /// If will_deretract(), executes the deretraction process and set retracted distance to unknown value (because it can be changed by printing moves without notice)
    void maybe_deretract_to_nozzle();

    /// Retracts the filament quickly to minimal distance, with no ramming
    /// @param purge_length if > 0.f, extrudes this much before retracting
    void ensure_retracted_no_ramming(float purge_length = 0.f);

    /// Save values to persistent storage
    void set_retracted_distance(PhysicalToolIndex tool, std::optional<float> distance);

private:
    AutoRetract();

    /// Shadows the config_store variable to reduce mutex locking
    std::bitset<PhysicalToolIndex::count> retracted_hotends_bitset_ = 0;

    /// Keeps whether saved value in persistent storage is known or unknown (invalidated)
    std::bitset<PhysicalToolIndex::count> known_hotends_bitset_ = 0;

    bool is_checking_deretract_ = false;
};

AutoRetract &auto_retract();

} // namespace buddy
