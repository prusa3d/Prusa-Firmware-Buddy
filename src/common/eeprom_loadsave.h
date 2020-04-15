// eeprom_loadsave.h

#ifndef _EEPROM_LOADSAVE_H
#define _EEPROM_LOADSAVE_H

#include "eeprom.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// load binary image (0x0000..0x0800) from usb flash (fatfs file)
extern int eeprom_load_bin_from_usb(const char *fn);

// save binary image (0x0000..0x0800) to usb flash (fatfs file)
extern int eeprom_save_bin_to_usb(const char *fn);

// TODO:
extern int eeprom_load_xml_from_usb(const char *fn);

// save xml file with all variables to usb flash (fatfs file)
extern int eeprom_save_xml_to_usb(const char *fn);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_EEPROM_LOADSAVE_H
