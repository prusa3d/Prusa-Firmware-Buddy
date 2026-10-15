#include "filament_tracker.hpp"

#include <algorithm>
#include <cmath>

using namespace buddy;

FilamentTracker::FilamentTracker() {
    // Start assuming all filament is unretracted
    // This is not necessarily true
    retracted_distances.fill(0);
}

void FilamentTracker::track_extruder_move(VirtualToolIndex virtual_tool, const ExtruderMove &move) {
    const PhysicalToolIndex physical_tool = virtual_tool.to_physical();

    // The retracted distance stays in commanded mm: auto_retract replays it as commanded moves.
    // The clamp below is a physical length compared against commanded mm; the few % mismatch is accepted.
    const float new_retracted_distance_unclamped = retracted_distances[physical_tool] - /* inverting to positive */ move.commanded_mm;

    // Update retracted distance
    {
        const float new_retraction_distance = std::clamp(new_retracted_distance_unclamped, 0.0f, extruder_to_nozzle_distance);
        retracted_distances[physical_tool] = new_retraction_distance;

        decltype(distance_valid)::reference valid = distance_valid[physical_tool.to_raw()];

        if (!valid && new_retraction_distance == 0.0f) {
            // we are now sure filament is fully in the nozzle
            valid = true;

        } else if (valid && new_retraction_distance == extruder_to_nozzle_distance) {
            // if we retract more than extruder_to_nozzle_distance, we most likely lost the track of the retractions, because the filament is no longer engaged with the extruder
            valid = false;
        }
    }

    // Update extruded distance - in mm actually fed
    {
        auto &edist = extruded_distances[virtual_tool];
        float integral;
        edist.fractional += std::max<float>(-new_retracted_distance_unclamped, 0) * move.feed_ratio;
        edist.fractional = std::modf(edist.fractional, &integral);
        edist.integral += static_cast<uint32_t>(integral);
    }
}

void buddy::FilamentTracker::assume_retracted_distance(PhysicalToolIndex physical_tool, std::optional<float> distance) {
    const float val = std::clamp<float>(distance.value_or(extruder_to_nozzle_distance), 0, extruder_to_nozzle_distance);
    retracted_distances[physical_tool] = val;
    distance_valid.set(physical_tool.to_raw(), val != extruder_to_nozzle_distance);
}

std::optional<float> FilamentTracker::get_retracted_distance(PhysicalToolIndex physical_tool) const {
    if (!distance_valid.test(physical_tool.to_raw())) {
        return std::nullopt;
    }
    return retracted_distances[physical_tool];
}

uint32_t buddy::FilamentTracker::get_extruded_distance(VirtualToolIndex virtual_tool) const {
    const auto &ed = extruded_distances[virtual_tool];
    return ed.integral;
}

FilamentTracker &buddy::filament_tracker() {
    static FilamentTracker instance;
    return instance;
}
