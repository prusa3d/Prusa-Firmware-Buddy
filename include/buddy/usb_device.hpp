#pragma once
#include <device/board.h>
#include "log.h"

extern log_component_t LOG_COMPONENT(USBDevice);

void usb_device_init();

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
void check_usb_connection();
#else
static constexpr void check_usb_connection() {}; // stub
#endif
