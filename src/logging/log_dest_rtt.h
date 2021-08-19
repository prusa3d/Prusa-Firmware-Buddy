#pragma once
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Send the event via Segger's RTT (in a non-blocking way, so data can get lost)
void rtt_log_event(log_destination_t *destination, log_event_t *event);

#ifdef __cplusplus
}
#endif //__cplusplus
