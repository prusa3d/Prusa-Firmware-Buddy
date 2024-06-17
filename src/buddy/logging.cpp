#include <stddef.h>

#include "logging.h"
#include "usb_device.hpp"

#include "log.h"
#include "log_dest_syslog.h"
#include "log_dest_shared.h"
#include "log_dest_rtt.h"
#include "log_dest_usb.h"

void logging_init() {
    static log_destination_t log_destination_rtt = {
        .lowest_severity = LOG_SEVERITY_DEBUG,
        .log_event_fn = rtt_log_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_rtt);

    static log_destination_t log_destination_syslog = {
        .lowest_severity = LOG_SEVERITY_INFO,
        .log_event_fn = syslog_log_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_syslog);

    static log_destination_t log_destination_usb = {
        .lowest_severity = LOG_SEVERITY_DEBUG,
        .log_event_fn = usb_log_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_usb);
}
