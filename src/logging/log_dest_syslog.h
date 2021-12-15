#pragma once
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Initialize the Syslog handler
///
/// Has to be called from a running thread
void syslog_initialize();

/// Format an event according to RFC5424
///
/// https://tools.ietf.org/html/rfc5424
void syslog_format_event(log_event_t *event, void (*out_fn)(char character, void *arg), void *arg);

/// Log an event using Syslog
///
/// The SYSLOG destination does not guarantee delivery of all events.
/// Events from ISRs are not being delivered, syslog won't try to send an event if running low on stack,
/// the UDP packet can be lost, etc.
///
/// The `destination` is expected to be configured to use the
/// `syslog_format_event` message formatter.
void syslog_log_event(log_destination_t *destination, log_event_t *event);

#ifdef __cplusplus
}
#endif //__cplusplus
