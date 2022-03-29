#include <variant>
#include "connect_error.h"
#include "core_interface.hpp"

namespace con {

class httpc_data {
public:
    // Fills provided buffer with telemetry data
    std::variant<size_t, Error> telemetry(device_params_t *params, char *buffer, size_t buffer_len);
    // Fills provided buffer with "INFO" event data
    std::variant<size_t, Error> info(printer_info_t *info, char *dest, const uint32_t buf_len, uint32_t command_id);
};

}
