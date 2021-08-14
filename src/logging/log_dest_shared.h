#pragma once
#include "log.h"
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void log_format_simple(log_event_t *event, void (*out_fn)(char character, void *arg), void *arg);

#ifdef __cplusplus
}
#endif //__cplusplus
