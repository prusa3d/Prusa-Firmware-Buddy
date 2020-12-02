// eeprom.h

#pragma once

#include <stdbool.h>
#include "variant8.h"

enum {
    EEPROM_ADDRESS = 0x0500, // uint16_t
    EEPROM_VERSION = 9,      // uint16_t
};

#define EEPROM_FEATURE_PID_NOZ 0x0001
#define EEPROM_FEATURE_PID_BED 0x0002
#define EEPROM_FEATURE_LAN     0x0004
#define EEPROM_FEATURE_SHEETS  0x0008
#define EEPROM_FEATURES        (EEPROM_FEATURE_PID_NOZ | EEPROM_FEATURE_PID_BED | EEPROM_FEATURE_LAN | EEPROM_FEATURE_SHEETS)

enum {
    MAX_SHEET_NAME_LENGTH = 8,
};

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
    EEVAR_MENU_TIMEOUT = 0x1e, // uint8_t on / off menu timeout flag
    EEVAR_ACTIVE_SHEET = 0x1f,
    EEVAR_SHEET_PROFILE0 = 0x20,
    EEVAR_SHEET_PROFILE1 = 0x21,
    EEVAR_SHEET_PROFILE2 = 0x22,
    EEVAR_SHEET_PROFILE3 = 0x23,
    EEVAR_SHEET_PROFILE4 = 0x24,
    EEVAR_SHEET_PROFILE5 = 0x25,
    EEVAR_SHEET_PROFILE6 = 0x26,
    EEVAR_SHEET_PROFILE7 = 0x27,
    EEVAR_SELFTEST_RESULT = 0x28, // uint32_t, two bits for each selftest part
    EEVAR_QR_PRIVACY = 0x29,      // uint8_t on / off sending UID in QR
    EEVAR__PADDING = 0x2a,        // 1..4 chars, to ensure (DATASIZE % 4 == 0)
    EEVAR_CRC32 = 0x2b,           // uint32_t crc32 for
};

enum {
    LAN_HOSTNAME_MAX_LEN = 20,
    CONNECT_TOKEN_SIZE = 20,
    LAN_EEFLG_ONOFF = 1, //EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
    LAN_EEFLG_TYPE = 2,  //EEPROM flag for user-defined settings (Switch between dhcp and static)
};

#define SelftestResult_Unknown 0
#define SelftestResult_Skipped 1
#define SelftestResult_Passed  2
#define SelftestResult_Failed  3

typedef union _SelftestResultEEprom_t {
    struct {
        uint8_t fan0 : 2;       // bit 0-1
        uint8_t fan1 : 2;       // bit 2-3
        uint8_t xaxis : 2;      // bit 4-5
        uint8_t yaxis : 2;      // bit 6-7
        uint8_t zaxis : 2;      // bit 8-9
        uint8_t nozzle : 2;     // bit 10-11
        uint8_t bed : 2;        // bit 12-13
        uint32_t reserved : 18; // bit 14-31
    };
    uint32_t ui32;
} SelftestResultEEprom_t;

enum {
    EEPROM_INIT_Normal = 0,
    EEPROM_INIT_Defaults = 1,
    EEPROM_INIT_Upgraded = 2
};

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// initialize eeprom
/// @returns 0 - normal init (eeprom data valid)
///          1 - defaults loaded
///          2 - eeprom upgraded successfully from a previous version
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

///////////////////////////////////////////////////////////////////////////////
/// @brief Iterate across the profiles and switch to the next calibrated.
///
/// Printer use print sheet profile on the index 0 as a default so the method
/// in the worst case iterate across entire profiles and return index 0 when
/// not any other profile is calibrated yet.
/// @return Index of the next calibrated profile.
extern uint32_t sheet_next_calibrated();

///////////////////////////////////////////////////////////////////////////////
/// @brief Determine if the given sheet profile is calibrated.
///
/// In case the index of the given print sheet profile is bigger than the
/// MAX_SHEETS method return false.
/// @param[in] index Index of the sheet profile
/// @return True when the profile is calibrated, False othewise.
extern bool sheet_is_calibrated(uint32_t);

///////////////////////////////////////////////////////////////////////////////
/// @brief Select the given print sheet profile as an active for the printer.
///
/// In case the index of the given print sheet profile is bigger than the
/// MAX_SHEETS or sheet is not calibrated method return false.
/// @param[in] index Index of the sheet profile
/// @return True when the profile can be selected, False othewise.
extern bool sheet_select(uint32_t);

///////////////////////////////////////////////////////////////////////////////
/// @brief Calibrate the given print sheet profile as an active for the printer.
///
/// In case the index of the given print sheet profile is bigger than the
/// MAX_SHEETS method return false.
/// @param[in] index Index of the sheet profile
/// @return True when the profile can be selected, False othewise.
extern bool sheet_calibrate(uint32_t);

///////////////////////////////////////////////////////////////////////////////
/// @brief Reset the given print sheet profile to the uncalibrated state.
///
/// In case the index of the given print sheet profile is bigger than the
/// MAX_SHEETS method return false.
/// @param[in] index Index of the sheet profile
/// @return True when the profile was reset, False othewise.
extern bool sheet_reset(uint32_t);

///////////////////////////////////////////////////////////////////////////////
/// @brief Reset the given print sheet profile to the uncalibrated state.
///
/// Printer use print sheet profile on the index 0 as a default so the method
/// return always at least 1 calibrated profile.
/// @return Return the count of the calibrated print sheet profiles.
extern uint32_t sheet_number_of_calibrated();

///////////////////////////////////////////////////////////////////////////////
/// @brief Determine the name of the current active print sheet profile.
///
/// @param[out] buffer Buffer to store the print sheet profile
/// @param[in] length Size of the given buffer.
/// @return Number of characters written to the buffer. Number will be
///        always less than MAX_SHEET_NAME_LENGTH
extern uint32_t sheet_active_name(char *, uint32_t);

///////////////////////////////////////////////////////////////////////////////
/// @brief Determine the name of the given print sheet profile.
///
/// @param[in] index Index of the sheet profile
/// @param[out] buffer Buffer to store the print sheet profile
/// @param[in] length Size of the given buffer.
/// @return Number of characters written to the buffer. Number will be
///        always less than MAX_SHEET_NAME_LENGTH
extern uint32_t sheet_name(uint32_t, char *, uint32_t);

///////////////////////////////////////////////////////////////////////////////
/// @brief Rename the given print sheet profile.
///
/// @param[in] index Index of the sheet profile
/// @param[in] buffer New name of the print sheet profile
/// @param[in] length Size of the given name.
/// @return Number of characters written to the buffer. Number will be
///        always less than MAX_SHEET_NAME_LENGTH
extern uint32_t sheet_rename(uint32_t, char const *, uint32_t);

#ifdef __cplusplus
}
#endif //__cplusplus
