// eeprom_loadsave.h

#pragma once

#include "eeprom.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// load binary image (0x0000..0x0800) from usb flash (fatfs file)
extern int eeprom_load_bin_from_usb(const char *fn);

// save binary image (0x0000..0x0800) to usb flash (fatfs file)
extern int eeprom_save_bin_to_usb(const char *fn);

#ifdef __cplusplus
}
#endif //__cplusplus
