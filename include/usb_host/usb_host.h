#ifndef __USB_HOST__H__
#define __USB_HOST__H__

#ifdef __cplusplus

namespace usbh_power_cycle {

void init();

/// callback from media_loop when printing is paused
void media_state_error();

/// callback from USBH_MSC_Worker when an io error occurs
void io_error();

/// callback from isr, it is called when the USB is disconnected or when USB flash is deadlocked
void port_disabled();
} // namespace usbh_power_cycle

extern "C" {
#endif

#include "usbh_def.h"

typedef enum {
    APPLICATION_IDLE = 0,
    APPLICATION_START,
    APPLICATION_READY,
    APPLICATION_DISCONNECT
} ApplicationTypeDef;

void MX_USB_HOST_Init(void);

bool device_connected_at_startup();
void USBH_UserProcess(USBH_HandleTypeDef *, uint8_t id);
extern TimerHandle_t USBH_restart_timer;

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST__H__ */
