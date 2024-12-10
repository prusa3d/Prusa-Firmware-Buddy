#ifndef __USB_HOST__H__
#define __USB_HOST__H__

#ifdef __cplusplus

namespace usb_host {

/// \returns whether there is a USB drive inserted in the USB slot
bool is_media_inserted();

/// \returns whether there is a USB drive connected since startup
bool is_media_inserted_since_startup();

// In case media is invalid, this will disable it until reconnection
void disable_media();
} // namespace usb_host

namespace usbh_power_cycle {

void init();

// callback from USBH_MSC_Worker when an io error occurs
void io_error();

// callback from isr, it is called when the USB is disconnected or when USB flash is deadlocked
void port_disabled();

// indication that the one click dialog during USB recovery reset should be blocked
bool block_one_click_print();

} // namespace usbh_power_cycle

extern "C" {
#endif

#include "usbh_def.h"

void MX_USB_HOST_Init(void);

void USBH_UserProcess(USBH_HandleTypeDef *, uint8_t id);
extern TimerHandle_t USBH_restart_timer;

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST__H__ */
