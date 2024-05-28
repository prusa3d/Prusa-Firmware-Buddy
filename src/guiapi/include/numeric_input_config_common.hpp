#include "numeric_input_config.hpp"

namespace numeric_input_config {

/// Config for entering a TCP/UDP port
static constexpr NumericInputConfig network_port = {
    .min_value = 0,
    .max_value = 65535,
};

} // namespace numeric_input_config
