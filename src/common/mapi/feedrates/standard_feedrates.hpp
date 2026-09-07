/// @file standard_feedrates.hpp

#pragma once

#include <core/types.h>
#include <filament.hpp>

namespace buddy {

/// Scales an extruder-move feedrate for the loaded filament's properties.
/// Currently only slows flexible filaments down (they buckle/grind in the extruder if pushed too fast);
/// This is the single place to add any future filament-dependent feedrate rule.
feedRate_t adjust_feedrate_for_filament(feedRate_t base, const FilamentTypeParameters &filament);

/// Scales an extruder-move feedrate for the loaded filament's properties.
/// Currently only slows flexible filaments down (they buckle/grind in the extruder if pushed too fast);
inline feedRate_t adjust_feedrate_for_filament(feedRate_t base, FilamentType filament) {
    return adjust_feedrate_for_filament(base, filament.parameters());
}

namespace standard_feedrates {

    /// Standard extrude operations
    enum class Extruder {
        pause_prime = 0, // Feedrate for priming after filament change.
        retract, // Feedrate for a small retract from nozzle during pause/park/...
        deretract, // Feedrate of inserting filament back into a nozzle after pause/park.
        filament_unload, // Unload filament feedrate.
        filament_slow_load, // Slow move when starting filament load.
        filament_fast_load, // Feedrate for pushing newly inserted filament into a hot extruder
        filament_assisted, // Feedrate to assist with filament insertion/removal/sample acquisition.
        advanced_pause_purge, // Purge feedrate (after loading). Should be slower than load feedrate.
        _count
    };

    /// @brief Standard feed rate for given situation/filament
    /// @param use_case
    /// @param filament
    /// @return Positive feedrate in mm/s
    feedRate_t extruder(Extruder use_case, const FilamentTypeParameters &filament);

    /// @brief Standard feed rate for given situation/filament
    /// @param use_case
    /// @param filament
    /// @return Positive feedrate in mm/s
    inline feedRate_t extruder(Extruder use_case, FilamentType filament) {
        return extruder(use_case, filament.parameters());
    };

    /// @brief Standard feedrate for filament currently loaded in a nozzle
    /// @param use_case
    /// @return Positive feedrate in mm/s
    inline feedRate_t current_extruder(Extruder use_case) {
        return extruder(use_case, FilamentType::for_current_tool_heuristic().parameters());
    };
}; // namespace standard_feedrates

} // namespace buddy
