#ifndef __USBH_CONF__H__
#define __USBH_CONF__H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <buddy/main.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#define USBH_MAX_NUM_ENDPOINTS 2

#define USBH_MAX_NUM_INTERFACES 2

#define USBH_MAX_NUM_CONFIGURATION 1

#define USBH_KEEP_CFG_DESCRIPTOR 1

#define USBH_MAX_NUM_SUPPORTED_CLASS 1

#define USBH_MAX_SIZE_CONFIGURATION 256

#define USBH_MAX_DATA_BUFFER 512

#define USBH_USE_OS 1

#define USBH_USE_MSC_CLASS_EVENTS 0

#define USBH_USE_ACK_INTERRUPTS 0

#define USBH_USE_IN_CHANNEL_BULK_NACK_INTERRUPTS 0

#define USBH_USE_URB_EVENTS 0

#define USBH_MSC_TRANSFER_SIZE 512

#define USBH_MSC_IO_TIMEOUT (25 * 1000)

#define MAX_SUPPORTED_LUN 1

/****************************************/
/* #define for FS and HS identification */
#define HOST_HS 0
#define HOST_FS 1

#if (USBH_USE_OS == 1)
    #include "cmsis_os.h"
    #include "buddy/priorities_config.h"
    #define USBH_PROCESS_PRIO       TASK_PRIORITY_USB_HOST
    #define USBH_PROCESS_STACK_SIZE ((uint16_t)320)
#endif /* (USBH_USE_OS == 1) */

/* Memory management macros */

/** Alias for memory allocation. */
#define USBH_malloc malloc

/** Alias for memory release. */
#define USBH_free free

/** Alias for memory set. */
#define USBH_memset memset

/** Alias for memory copy. */
#define USBH_memcpy memcpy

/* DEBUG macros */
void USBH_UsrLog(const char *fmt, ...);
void USBH_ErrLog(const char *fmt, ...);
void USBH_DbgLog(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __USBH_CONF__H__ */
