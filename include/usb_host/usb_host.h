#ifndef __USB_HOST__H__
#define __USB_HOST__H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_def.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

typedef enum {
    APPLICATION_IDLE = 0,
    APPLICATION_START,
    APPLICATION_READY,
    APPLICATION_DISCONNECT
} ApplicationTypeDef;

void MX_USB_HOST_Init(void);

bool device_connected_at_startup();
void USBH_UserProcess(USBH_HandleTypeDef *, uint8_t id);
void USBH_MSC_StealthReset(USBH_HandleTypeDef *, uint8_t lun);

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST__H__ */
