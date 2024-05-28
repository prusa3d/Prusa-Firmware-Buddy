#pragma once

#include <logging/log.hpp>

namespace logging {

/// Set up the syslog handler based on config store
void syslog_reconfigure();

/// Format an event according to RFC5424
///
/// https://tools.ietf.org/html/rfc5424
void syslog_format_event(Event *event, void (*out_fn)(char character, void *arg), void *arg);

/// Log an event using Syslog
///
/// The SYSLOG destination does not guarantee delivery of all events.
/// Events from ISRs are not being delivered, syslog won't try to send an event if running low on stack,
/// the UDP packet can be lost, etc.
void syslog_log_event(FormattedEvent *event);

} // namespace logging
