#include <stddef.h>

#include <buddy/logging.h>
#include <buddy/usb_device.hpp>

#include <logging/log_dest_rtt.hpp>
#include <logging/log_dest_syslog.hpp>
#include <logging/log_dest_usb.hpp>
#include <logging/log_dest_file.hpp>
#include <logging/log.hpp>

void logging_init() {
    static logging::Destination log_destination_rtt = {
        .lowest_severity = logging::Severity::debug,
        .log_event_fn = logging::rtt_log_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_rtt);

    static logging::Destination log_destination_syslog = {
        .lowest_severity = logging::Severity::info,
        .log_event_fn = logging::syslog_log_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_syslog);

    static logging::Destination log_destination_usb = {
        .lowest_severity = logging::Severity::debug,
        .log_event_fn = logging::usb_log_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_usb);

    static logging::Destination log_destination_file = {
        .lowest_severity = logging::Severity::debug,
        .log_event_fn = logging::file_log_event,
        .next = NULL,
    };
    log_destination_register(&log_destination_file);
}
