#include "logging.hpp"

#include "logging/log.hpp"
#include "logging/log_dest_cyphal.hpp"

void anfc::logging_init() {
    // Cyphal is the only destination here. RTT would not fit - the STM32C0 has 30 kB of RAM and its
    // buffers cost more than what is left after the Cyphal Record buffers.
    static logging::Destination log_destination_cyphal = {
        .lowest_severity = logging::Severity::info,
        .log_event_fn = cyphal_log_event,
        .next = nullptr,
    };
    log_destination_register(&log_destination_cyphal);
}
