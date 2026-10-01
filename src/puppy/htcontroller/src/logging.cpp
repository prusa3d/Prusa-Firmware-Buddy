#include "logging.hpp"
#include "timing.h"

#include <logging/log.hpp>
#include <logging/log_dest_rtt.hpp>
#include <logging/log_platform.hpp>

void htcontroller::logging_init() {
    static logging::Destination log_destination_rtt = {
        .lowest_severity = logging::Severity::debug,
        .log_event_fn = logging::rtt_log_event,
        .next = nullptr,
    };
    log_destination_register(&log_destination_rtt);
}

namespace logging {

Timestamp log_platform_timestamp_get() {
    const uint32_t ms = ticks_ms();
    return { ms / 1000, (ms % 1000) * 1000 };
}

TaskId log_platform_task_id_get() {
    // Task numbers need configUSE_TRACE_FACILITY, which is off to save RAM
    return -1;
}

} // namespace logging
