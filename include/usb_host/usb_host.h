#ifndef __USB_HOST__H__
#define __USB_HOST__H__

#ifdef __cplusplus

namespace usbh_power_cycle {

void init();

// callback from USBH_MSC_Worker when an io error occurs
void io_error();

// callback from isr, it is called when the USB is disconnected or when USB flash is deadlocked
void port_disabled();

// indication that the one click dialog during USB recovery reset should be blocked
bool block_one_click_print();

// indication that a USB error dialog should be displayed
// usb reset was unsuccessful and nothing else remains after emptying the prefetch buffer
extern std::atomic<bool> trigger_usb_failed_dialog;

} // namespace usbh_power_cycle

extern "C" {
#endif

#include "usbh_def.h"

void MX_USB_HOST_Init(void);

bool device_connected_at_startup();
void USBH_UserProcess(USBH_HandleTypeDef *, uint8_t id);
extern TimerHandle_t USBH_restart_timer;

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST__H__ */
