// eeprom.h

#ifndef _EEPROM_H
#define _EEPROM_H

#include "variant8.h"

#define EEPROM_VERSION        3


#define EEVAR_VERSION         0x00
#define EEVAR_FILAMENT_TYPE   0x01
#define EEVAR_FILAMENT_COLOR  0x02
#define EEVAR_UNUSED_1        0x03
#define EEVAR_UNUSED_2        0x04
#define EEVAR_UNUSED_3        0x05
#define EEVAR_RUN_SELFTEST    0x06
#define EEVAR_RUN_XYZCALIB    0x07
#define EEVAR_RUN_FIRSTLAY    0x08
#define EEVAR_FSENSOR_ENABLED 0x09
#define EEVAR_ZOFFSET         0x0a
#define EEVAR_PID_NOZ_P       0x0b
#define EEVAR_PID_NOZ_I       0x0c
#define EEVAR_PID_NOZ_D       0x0d
#define EEVAR_PID_BED_P       0x0e
#define EEVAR_PID_BED_I       0x0f
#define EEVAR_PID_BED_D       0x10
#define EEVAR_TEST            0x11


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


// initialize eeprom
extern uint8_t eeprom_init(void);

// write default values to all variables
extern void eeprom_defaults(void);

// get variable value as variant8
extern variant8_t eeprom_get_var(uint8_t id);

// set variable value as variant8
extern void eeprom_set_var(uint8_t id, variant8_t var);

// fill range 0x0000..0x0800 with 0xff
extern void eeprom_clear(void);

int8_t eeprom_test_PUT(const unsigned int);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_EEPROM_H
