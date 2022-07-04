#include <variant>
#include "connect_error.h"
#include "core_interface.hpp"

// TODO: The concept here & around the core_interface is a bit weird. It
// doesn't seem to fullfill the goal of abstracting the printer away and needs
// the whole buffer all at once.

namespace con {

class httpc_data {
public:
    // Fills provided buffer with telemetry data
    std::variant<size_t, Error> telemetry(const device_params_t &params, char *buffer, size_t buffer_len);
    // Fills provided buffer with "INFO" event data
    std::variant<size_t, Error> info(const printer_info_t &info, char *dest, const uint32_t buf_len, uint32_t command_id);
    // The JOB_INFO event.
    std::variant<size_t, Error> job_info(const device_params_t &params, char *dest, const uint32_t buf_len, uint32_t command_id);
};

}
