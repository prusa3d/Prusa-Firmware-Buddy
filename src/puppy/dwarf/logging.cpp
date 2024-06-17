#include "logging.hpp"

#include "log.h"
#include "log_dest_shared.h"
#include "log_dest_rtt.h"
#include "log_dest_bufflog.h"

void dwarf::logging_init() {
    static log_destination_t log_destination_rtt = {
        .lowest_severity = LOG_SEVERITY_DEBUG,
        .log_event_fn = rtt_log_event,
        .next = nullptr,
    };
    log_destination_register(&log_destination_rtt);

    static log_destination_t log_destination_bufflog = {
        .lowest_severity = LOG_SEVERITY_INFO,
        .log_event_fn = bufflog_log_event,
        .next = nullptr,
    };
    log_destination_register(&log_destination_bufflog);
}
