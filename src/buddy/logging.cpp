#include <stddef.h>

#include "logging.h"
#include "usb_device.hpp"

#include "log.h"
#include "log_dest_swo.h"
#include "log_dest_syslog.h"
#include "log_dest_shared.h"
#include "log_dest_rtt.h"
#include "log_dest_usb.h"

void logging_init() {
    static log_destination_t log_destination_rtt = {
        .name = "RTT",
        .lowest_severity = LOG_SEVERITY_DEBUG,
        .log_event_fn = rtt_log_event,
        .log_format_fn = log_format_simple,
        .next = NULL,
    };
    log_destination_register(&log_destination_rtt);

    static log_destination_t log_destination_swo = {
        .name = "SWO",
        .lowest_severity = LOG_SEVERITY_DEBUG,
        .log_event_fn = swo_log_event,
        .log_format_fn = log_format_simple,
        .next = NULL,
    };
    log_destination_register(&log_destination_swo);

    static log_destination_t log_destination_syslog = {
        .name = "SYSLOG",
        .lowest_severity = LOG_SEVERITY_INFO,
        .log_event_fn = syslog_log_event,
        .log_format_fn = syslog_format_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_syslog);

    static log_destination_t log_destination_usb = {
        .name = "USB",
        .lowest_severity = LOG_SEVERITY_DEBUG,
        .log_event_fn = usb_log_event,
        .log_format_fn = log_format_simple,
        .next = NULL,
    };
    log_destination_register(&log_destination_usb);
}
