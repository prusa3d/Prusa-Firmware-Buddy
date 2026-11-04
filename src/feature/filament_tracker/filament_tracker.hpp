/// \file

#pragma once

#include <utils/uncopyable.hpp>
#include <utils/mutex_atomic.hpp>
#include <freertos/mutex.hpp>
#include <bitset>
#include <cstdint>
#include <optional>
#include <option/has_nextruder.h>
#include <option/has_indx_head.h>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>

namespace buddy {

/**
 *  Passive observer that keeps track of retracted distance on each hotend during printing
 *
 *  On XL, retracting and ramming is handled by PrusaSlicer, which invalidates filament_retracted_distances in persistent storage
 *  and therefore collides with auto_retract feature. This tracker keeps track of the retracted distance on each hotend during printing
 *  and on print finish/abort/stop it saves the values of respected hotends in the persistent storage
 *
 *  Retracted distances are saved in range < 0 ; 254 > (mm)
 *  Value 255 is reserved as invalid / unknown value
 */
class FilamentTracker : Uncopyable {
    friend FilamentTracker &filament_tracker();

public:
#if HAS_NEXTRUDER()
    static constexpr float extruder_to_nozzle_distance = 40.f; // mm
#elif HAS_INDX_HEAD()
    static constexpr float extruder_to_nozzle_distance = 27.f; // mm
#else
    #error
#endif

    struct ExtruderMove {
        /// Commanded E distance (mm), flow factor already applied
        float commanded_mm;

        /// Length the extruder actually feeds per commanded mm
        float feed_ratio;
    };

    /// Lets the tracker know about a planned extruder move
    /// The only place this should be called from is planner.buffer_segment
    void track_extruder_move(VirtualToolIndex virtual_tool, const ExtruderMove &move);

    /// @returns how much the filament is retracted  (in mm) from the nozzle of @p physical_tool
    ///   or std::nullopt if the value is not known
    std::optional<float> get_retracted_distance(PhysicalToolIndex physical_tool) const;

    /// Make the system assume that the specified tool has a filament retracted to the specified distance
    /// std::nullopt would signalize that the filament is fully removed
    void assume_retracted_distance(PhysicalToolIndex physical_tool, std::optional<float> distance);

    /// @returns how much filament (in mm) the extruder has actually fed out of the nozzle since last reset
    /// This value is NOT persistent
    uint32_t get_extruded_distance(VirtualToolIndex virtual_tool) const;

    /// @returns a random value that identifies the current run of the extruded distance counters.
    /// The counters restart from zero on every boot, and this value changes with them.
    /// Consumers computing usage deltas from @ref get_extruded_distance use it to detect the reset.
    uint32_t extruded_distance_id() const {
        return extruded_distance_id_;
    }

    struct Unloads {
        /// How many times the tool went from loaded to empty since boot
        uint32_t count = 0;

        /// @ref get_extruded_distance at the most recent of those; 0 if there was none
        uint32_t extruded_at_last = 0;
    };

    /// Lets the tracker know that the filament of @p virtual_tool went from loaded to empty
    void track_filament_unload(VirtualToolIndex virtual_tool);

    /// Unloads of @p virtual_tool since boot.
    /// Both fields are updated together, so they are always consistent with each other.
    /// To read a consistent pair with @ref get_extruded_distance, read the unloads first:
    /// the extruded distance never decreases, so it is then never below @ref Unloads::extruded_at_last.
    Unloads get_unloads(VirtualToolIndex virtual_tool) const;

#ifndef UNITTESTS
private:
#endif
    FilamentTracker();

private: // * per-PhysicalToolIndex things
    StrongIndexArray<float, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> retracted_distances;
    std::bitset<PhysicalToolIndex::count> distance_valid; ///< The hotend validates  by traveling at least +extruder_to_nozzle_distance

private: // * per-VirtualToolIndex things
    /// Planner reports the filament usage in possibly very small numbers
    /// To prevent accumulation errors, we've split the thing between two floats
    struct ExtrudedDistance {
        uint32_t integral = 0;
        float fractional = 0;
    };

    /// This is used for filament usage tracking, so it must be per VIRTUAL tool.
    StrongIndexArray<ExtrudedDistance, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> extruded_distances;

    /// Written by whichever task changes the loaded filament, read by API handlers
    MutexAtomic<StrongIndexArray<Unloads, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static>, freertos::Mutex> unloads;

    uint32_t extruded_distance_id_;
};

FilamentTracker &filament_tracker();

} // namespace buddy
