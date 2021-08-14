#pragma once
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void swo_log_event(log_destination_t *destination, log_event_t *event);

#ifdef __cplusplus
}
#endif //__cplusplus
