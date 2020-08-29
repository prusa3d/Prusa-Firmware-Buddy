// eeprom.h

#pragma once

#include "variant8.h"

enum {
    EEPROM_ADDRESS = 0x0500,
    EEPROM_VERSION = 8,
};

#define EEPROM_FEATURE_PID_NOZ 0x0001
#define EEPROM_FEATURE_PID_BED 0x0002
#define EEPROM_FEATURE_LAN     0x0004
#define EEPROM_FEATURES        (EEPROM_FEATURE_PID_NOZ | EEPROM_FEATURE_PID_BED | EEPROM_FEATURE_LAN)

enum {
    // basic variables
    EEVAR_VERSION = 0x00,         // uint16_t eeprom version
    EEVAR_FEATURES = 0x01,        // uint16_t feature mask
    EEVAR_DATASIZE = 0x02,        // uint16_t eeprom data size
    EEVAR_FW_VERSION = 0x03,      // uint16_t encoded firmware version (e.g. 403 for 4.0.3)
    EEVAR_FW_BUILD = 0x04,        // uint16_t firmware build number
    EEVAR_FILAMENT_TYPE = 0x05,   // uint8_t  filament type
    EEVAR_FILAMENT_COLOR = 0x06,  // uint32_t filament color (rgb)
    EEVAR_RUN_SELFTEST = 0x07,    // uint8_t  selftest flag
    EEVAR_RUN_XYZCALIB = 0x08,    // uint8_t  xyz calibration flag
    EEVAR_RUN_FIRSTLAY = 0x09,    // uint8_t  first layer calibration flag
    EEVAR_FSENSOR_ENABLED = 0x0a, // uint8_t  fsensor state
    EEVAR_ZOFFSET = 0x0b,         // float    zoffset

// nozzle PID variables
#if (EEPROM_FEATURES & EEPROM_FEATURE_PID_NOZ)
    EEVAR_PID_NOZ_P = 0x0c, // float    PID constants for nozzle
    EEVAR_PID_NOZ_I = 0x0d, // float
    EEVAR_PID_NOZ_D = 0x0e, // float
#endif

// bed PID variables
#if (EEPROM_FEATURES & EEPROM_FEATURE_PID_BED)
    EEVAR_PID_BED_P = 0x0f, // float    PID constants for bed
    EEVAR_PID_BED_I = 0x10, // float
    EEVAR_PID_BED_D = 0x11, // float
#endif

// lan variables
#if (EEPROM_FEATURES & EEPROM_FEATURE_LAN)
    EEVAR_LAN_FLAG = 0x12,     // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1
    EEVAR_LAN_IP4_ADDR = 0x13, // X.X.X.X address encoded in uint32
    EEVAR_LAN_IP4_MSK = 0x14,  // X.X.X.X address encoded in uint32
    EEVAR_LAN_IP4_GW = 0x15,   // X.X.X.X address encoded in uint32
    EEVAR_LAN_IP4_DNS1 = 0x16, // X.X.X.X address encoded in uint32
    EEVAR_LAN_IP4_DNS2 = 0x17, // X.X.X.X address encoded in uint32
    EEVAR_LAN_HOSTNAME = 0x18, // 20char string
    EEVAR_TIMEZONE = 0x19,     // int8_t hour difference from UTC
#endif                         // (EEPROM_FEATURES & EEPROM_FEATURE_LAN)

    // sound variable
    EEVAR_SOUND_MODE = 0x1a,   // uint8_t
    EEVAR_SOUND_VOLUME = 0x1b, // uint8_t
    EEVAR_LANGUAGE = 0x1c,     // uint16_t
    EEVAR_FILE_SORT = 0x1d,    // uint8_t  filebrowser file sort options
    EEVAR__PADDING = 0x1e,     // 1..4 chars, to ensure (DATASIZE % 4 == 0)
    EEVAR_CRC32 = 0x1f,        // uint32_t crc32 for
};

enum {
    LAN_HOSTNAME_MAX_LEN = 20,
    CONNECT_TOKEN_SIZE = 20,
    LAN_EEFLG_ONOFF = 1, //EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
    LAN_EEFLG_TYPE = 2,  //EEPROM flag for user-defined settings (Switch between dhcp and static)
};

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
