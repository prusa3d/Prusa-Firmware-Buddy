#pragma once
#include "log.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Initialize the buffered handler
///
/// Has to be called from a running thread
void bufflog_initialize();

/// Log an event using BuffLog
///
/// The BuffLog destination does not guarantee delivery of all events.
/// There is a limited buffer that can overflow. Messages might get logs on bus.
///
/// The `destination` is expected to be configured to use the
/// `buffbus_log_event` message formatter.
void bufflog_log_event(log_destination_t *destination, log_event_t *event);

/// Pipukup data from buffer
size_t bufflog_pickup(char *buffer, size_t buffer_size);

/// Terminate event record with EOT
#define BUFFLOG_TERMINATION_CHAR 4

#ifdef __cplusplus
}
#endif //__cplusplus
