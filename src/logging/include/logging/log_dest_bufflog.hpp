#pragma once

#include <logging/log.hpp>
#include <cstddef>

namespace logging {

/// Log an event using BuffLog
///
/// The BuffLog destination does not guarantee delivery of all events.
/// There is a limited buffer that can overflow. Messages might get logs on bus.
void bufflog_log_event(FormattedEvent *event);

/// Pipukup data from buffer
size_t bufflog_pickup(char *buffer, size_t buffer_size);

/// Terminate event record with EOT
#define BUFFLOG_TERMINATION_CHAR 4

} // namespace logging
