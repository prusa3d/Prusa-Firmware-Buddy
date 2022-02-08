#pragma once
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Send the event via USB CDC
void usb_log_event(log_destination_t *destination, log_event_t *event);

void usb_log_enable();

void usb_log_disable();

#ifdef __cplusplus
}
#endif //__cplusplus
