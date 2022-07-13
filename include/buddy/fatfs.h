#ifndef __fatfs_H
#define __fatfs_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "ff_gen_drv.h"
#include "usbh_diskio.h" /* defines USBH_Driver as external */

extern uint8_t retUSBH;  /* Return value for USBH */
extern char USBHPath[4]; /* USBH logical drive path */
extern FATFS USBHFatFS;  /* File system object for USBH logical drive */
extern FIL USBHFile;     /* File object for USBH */

void MX_FATFS_Init(void);

#ifdef __cplusplus
}
#endif

#endif /*__fatfs_H */
