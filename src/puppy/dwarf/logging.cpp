#include "logging.hpp"

#include <logging/log.hpp>
#include <logging/log_dest_rtt.hpp>
#include <logging/log_dest_bufflog.hpp>

void dwarf::logging_init() {
    static logging::Destination log_destination_rtt = {
        .lowest_severity = logging::Severity::debug,
        .log_event_fn = logging::rtt_log_event,
        .next = nullptr,
    };
    log_destination_register(&log_destination_rtt);

    static logging::Destination log_destination_bufflog = {
        .lowest_severity = logging::Severity::info,
        .log_event_fn = logging::bufflog_log_event,
        .next = nullptr,
    };
    log_destination_register(&log_destination_bufflog);
}
