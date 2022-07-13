#include "fatfs.h"
#include <wui_api.h>

uint8_t retUSBH;  /* Return value for USBH */
char USBHPath[4]; /* USBH logical drive path */
FATFS USBHFatFS;  /* File system object for USBH logical drive */
FIL USBHFile;     /* File object for USBH */

void MX_FATFS_Init(void) {
    retUSBH = FATFS_LinkDriver(&USBH_Driver, USBHPath);
}

/**
 * @brief  Gets Time from RTC
 * @param  None
 * @retval Time in DWORD
 */
DWORD get_fattime(void) {
    time_t timestamp = sntp_get_system_time();
    struct tm time;
    localtime_r(&timestamp, &time);

    return ((time.tm_year - 80) << 25) | ((time.tm_mon + 1) << 21) | (time.tm_mday << 16) | (time.tm_hour << 11) | (time.tm_min << 5) | (time.tm_sec >> 1);
}
