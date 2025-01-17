#include "numeric_input_config.hpp"

#include <option/has_chamber_api.h>

namespace numeric_input_config {

/// Config for entering a TCP/UDP port
static constexpr NumericInputConfig network_port = {
    .min_value = 0,
    .max_value = 65535,
};

/// Numeric config for setting the nozzle temperature: 0 - max temp, 0 = Off
extern const NumericInputConfig nozzle_temperature;

/// Numeric config for setting nozzle temperature for a filament type - EXTRUDE_MINTEMP - maxtemp
extern const NumericInputConfig filament_nozzle_temperature;

extern const NumericInputConfig bed_temperature;

/// 0-100 %, 0 % = off
extern const NumericInputConfig percent_with_off;

/// 0-100 %, -1 = auto
extern const NumericInputConfig percent_with_auto;

#if HAS_CHAMBER_API()
/// Degrees celsius.
/// This is a function because the config is dynamic and can change based on what chamber backend is currently running.
const NumericInputConfig &chamber_temp_with_off();

/// Degrees celsius.
/// This is a function because the config is dynamic and can change based on what chamber backend is currently running.
const NumericInputConfig &chamber_temp_with_none();
#endif

} // namespace numeric_input_config
