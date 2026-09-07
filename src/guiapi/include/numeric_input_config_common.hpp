#include "numeric_input_config.hpp"

#include <variant>

#include <option/has_chamber_api.h>
#include <option/has_heatbreak_temp.h>
#include <tool_index.hpp>

namespace numeric_input_config {

/// Config for entering a TCP/UDP port
static constexpr NumericInputConfig network_port = {
    .min_value = 0,
    .max_value = 65535,
};

/// Numeric config for setting the nozzle temperature: 0 - max temp, 0 = Off.
/// Returned by value: the caller must own it (e.g. via NumericInputConfigHolder). Different
/// tools may have different max temps, so a shared-static instance would be unsafe when several
/// per-tool spin items coexist.
/// @param tool A specific tool, or AllTools for the range covering every installed hotend.
NumericInputConfig nozzle_temperature(std::variant<PhysicalToolIndex, AllTools> tool);

/// Numeric config for a filament type's nozzle temperature: EXTRUDE_MINTEMP - max temp.
/// Returned by value, see nozzle_temperature().
/// @param tool A specific tool, or AllTools for the range covering every installed hotend.
NumericInputConfig filament_nozzle_temperature(std::variant<PhysicalToolIndex, AllTools> tool);

extern const NumericInputConfig bed_temperature;

#if HAS_HEATBREAK_TEMP()
extern const NumericInputConfig heatbreak_temperature;
#endif

/// 0-100 %, 0 % = off
extern const NumericInputConfig percent_with_off;

/// 0-100 %, -1 = auto
extern const NumericInputConfig percent_with_auto;

/// 0-100 %, -1 = disabled
/// (just to visually distinguish the case where we do have auto, but the auto
///  acts as always disabled due to other conditions).
extern const NumericInputConfig percent_with_disabled;

#if HAS_CHAMBER_API()
/// Degrees celsius.
/// This is a function because the config is dynamic and can change based on what chamber backend is currently running.
const NumericInputConfig &chamber_temp_with_off();

/// Degrees celsius.
/// This is a function because the config is dynamic and can change based on what chamber backend is currently running.
const NumericInputConfig &chamber_temp_with_none();
#endif

} // namespace numeric_input_config
