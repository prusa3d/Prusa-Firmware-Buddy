#pragma once

#include <option/has_nozzle_cleaner_lite.h>

#include <gcode/inject_queue_actions.hpp>
#include <optional>
#include <string_view>
#include <str_utils.hpp>

#include <core/types.h>
#include <mapi/parking.hpp>
#include <utils/badge.hpp>

class unified_bed_leveling;

namespace nozzle_cleaner_lite {

/// True when the running unit actually has a nozzle cleaner lite version enabled.
bool is_available();

/// Touchpoint cool-down temperature sits this much below the cleaning
/// temperature: no active ooze, yet as hot as possible so the nozzle
/// thermal expansion stays close to printing conditions. The probing tool
/// keeps it for the probing that follows.
constexpr int16_t cooldown_temp_diff = 20;
struct CleanArgs {
    // cleaning_temp below EXTRUDE_MINTEMP fails the clean.
    int16_t cleaning_temp;
    // target to cool down to, defaulting to
    // cleaning_temp - cooldown_temp_diff. Must be <= cleaning_temp.
    std::optional<int16_t> probe_temp;
    bool cooldown;
    bool keep_target;
};

/// Home if needed, probe the cleaner's touchpoint reference point, run the
/// clean cycle and rest on the touchpoint until the nozzle cools down.
/// Caller must check is_available() first.
bool clean(CleanArgs args);

} // namespace nozzle_cleaner_lite
