// eeprom.h

#ifndef _EEPROM_H
#define _EEPROM_H

#include "variant8.h"

#define EEPROM_ADDRESS  0x0500
#define EEPROM_VERSION  4
#define EEPROM_FEATURES (EEPROM_FEATURE_PID_NOZ | EEPROM_FEATURE_PID_BED | EEPROM_FEATURE_LAN)

#define EEPROM_FEATURE_PID_NOZ 0x0001
#define EEPROM_FEATURE_PID_BED 0x0002
#define EEPROM_FEATURE_LAN     0x0004

// basic variables
#define EEVAR_VERSION         0x00 // uint16_t eeprom version
#define EEVAR_FEATURES        0x01 // uint16_t feature mask
#define EEVAR_DATASIZE        0x02 // uint16_t eeprom data size
#define EEVAR_FW_VERSION      0x03 // uint16_t encoded firmware version (e.g. 403 for 4.0.3)
#define EEVAR_FW_BUILD        0x04 // uint16_t firmware build number
#define EEVAR_FILAMENT_TYPE   0x05 // uint8_t  filament type
#define EEVAR_FILAMENT_COLOR  0x06 // uint32_t filament color (rgb)
#define EEVAR_RUN_SELFTEST    0x07 // uint8_t  selftest flag
#define EEVAR_RUN_XYZCALIB    0x08 // uint8_t  xyz calibration flag
#define EEVAR_RUN_FIRSTLAY    0x09 // uint8_t  first layer calibration flag
#define EEVAR_FSENSOR_ENABLED 0x0a // uint8_t  fsensor state
#define EEVAR_ZOFFSET         0x0b // float    zoffset

// nozzle PID variables
#if (EEPROM_FEATURES & EEPROM_FEATURE_PID_NOZ)
    #define EEVAR_PID_NOZ_P 0x0c // float    PID constants for nozzle
    #define EEVAR_PID_NOZ_I 0x0d // float
    #define EEVAR_PID_NOZ_D 0x0e // float
#endif

// bed PID variables
#if (EEPROM_FEATURES & EEPROM_FEATURE_PID_BED)
    #define EEVAR_PID_BED_P 0x0f // float    PID constants for bed
    #define EEVAR_PID_BED_I 0x10 // float
    #define EEVAR_PID_BED_D 0x11 // float
#endif

// lan variables
#if (EEPROM_FEATURES & EEPROM_FEATURE_LAN)
    #define EEVAR_LAN_FLAG     0x12 // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1
    #define EEVAR_LAN_IP4_ADDR 0x13 // X.X.X.X address encoded in uint32
    #define EEVAR_LAN_IP4_MSK  0x14 // X.X.X.X address encoded in uint32
    #define EEVAR_LAN_IP4_GW   0x15 // X.X.X.X address encoded in uint32
    #define EEVAR_LAN_IP4_DNS1 0x16 // X.X.X.X address encoded in uint32
    #define EEVAR_LAN_IP4_DNS2 0x17 // X.X.X.X address encoded in uint32
    #define EEVAR_CONNECT_IP4  0x18 // X.X.X.X address encoded in uint32
    #define EEVAR_CONNECT_TOKEN 0x19 // 20char string
    #define EEVAR_LAN_HOSTNAME 0x1A // 20char string
#endif // (EEPROM_FEATURES & EEPROM_FEATURE_LAN)

#define EEVAR__PADDING 0x19 // 1..4 chars, to ensure (DATASIZE % 4 == 0)

#define EEVAR_CRC32 0x1a // uint32_t crc32 for

#define LAN_HOSTNAME_MAX_LEN 20
#define CONNECT_TOKEN_SIZE 20
#define LAN_EEFLG_ONOFF      1 //EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
#define LAN_EEFLG_TYPE       2 //EEPROM flag for user-defined settings (Switch between dhcp and static)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// initialize eeprom, return values:  1 - defaults loaded, 0 - normal init (eeprom data valid)
extern uint8_t eeprom_init(void);

// write default values to all variables
extern void eeprom_defaults(void);

// get variable value as variant8
extern variant8_t eeprom_get_var(uint8_t id);

// set variable value as variant8
extern void eeprom_set_var(uint8_t id, variant8_t var);

// get number of variables
extern uint8_t eeprom_get_var_count(void);

// get variable name
extern const char *eeprom_get_var_name(uint8_t id);

// format variable value to string (some variables can have specific formating)
extern int eeprom_var_format(char *str, unsigned int size, uint8_t id, variant8_t var);

// fill range 0x0000..0x0800 with 0xff
extern void eeprom_clear(void);

// PUT test
int8_t eeprom_test_PUT(const unsigned int);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_EEPROM_H
