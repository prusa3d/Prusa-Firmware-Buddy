#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "log.h"

extern log_component_t LOG_COMPONENT(USBDevice);

void usb_device_init();

#ifdef __cplusplus
}
#endif
