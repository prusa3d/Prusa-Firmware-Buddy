#include "numeric_input_config.hpp"

namespace numeric_input_config {

/// Config for entering a TCP/UDP port
static constexpr NumericInputConfig network_port = {
    .min_value = 0,
    .max_value = 65535,
};

extern const NumericInputConfig nozzle_temperature;
extern const NumericInputConfig bed_temperature;

} // namespace numeric_input_config
