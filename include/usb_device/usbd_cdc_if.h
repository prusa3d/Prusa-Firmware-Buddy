#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "usbd_cdc.h"

#ifdef USE_USB_FS
    #define CDC_DATA_MAX_PACKET_SIZE CDC_DATA_FS_MAX_PACKET_SIZE
#else
    #define CDC_DATA_MAX_PACKET_SIZE CDC_DATA_HS_MAX_PACKET_SIZE
#endif

enum {
    /// Receive data size
    USBD_CDC_RX_DATA_SIZE = 0,
    /// Transmit buffef size (we need a space for one packet)
    USBD_CDC_TX_DATA_SIZE = CDC_DATA_MAX_PACKET_SIZE,
};

_Static_assert(USBD_CDC_TX_DATA_SIZE >= CDC_DATA_MAX_PACKET_SIZE, "tx buffer should hold at least 1 full usb packet");

/// USB Device CDC Interface
extern USBD_CDC_ItfTypeDef usbd_cdc_if;

/// Check whether the interface has been initialized
bool usbd_cdc_is_ready();

/// Send data over the CDC interface
uint8_t usbd_cdc_transmit(uint8_t *buffer, uint32_t length);

/// Register a function to be called with received data
void usbd_cdc_register_receive_fn(void (*receive_fn)(uint8_t *buffer, uint32_t length));

#ifdef __cplusplus
}
#endif
