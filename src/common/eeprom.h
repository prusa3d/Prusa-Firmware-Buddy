// eeprom.h

#pragma once

#include <stdbool.h>
#include "variant8.h"
#include "eeprom_function_api.h"

enum {
    EEPROM_ADDRESS = 0x0500, // uint16_t
    EEPROM_VERSION = 11,     // uint16_t
};

#define EEPROM_LAST_VERSION_WITH_OLD_CRC 10

#define EEPROM_FEATURE_PID_NOZ 0x0001
#define EEPROM_FEATURE_PID_BED 0x0002
#define EEPROM_FEATURE_LAN     0x0004
#define EEPROM_FEATURE_SHEETS  0x0008
#define EEPROM_FEATURES        (EEPROM_FEATURE_PID_NOZ | EEPROM_FEATURE_PID_BED | EEPROM_FEATURE_LAN | EEPROM_FEATURE_SHEETS)

enum {
    MAX_SHEET_NAME_LENGTH = 8,
};

enum eevar_id {
    // basic variables
    EEVAR_VERSION = 0x00,                     // uint16_t eeprom version
    EEVAR_FEATURES = 0x01,                    // uint16_t feature mask
    EEVAR_DATASIZE = 0x02,                    // uint16_t eeprom data size
    EEVAR_FW_VERSION = 0x03,                  // uint16_t encoded firmware version (e.g. 403 for 4.0.3)
    EEVAR_FW_BUILD = 0x04,                    // uint16_t firmware build number
    EEVAR_FILAMENT_TYPE = 0x05,               // uint8_t  filament type
    EEVAR_FILAMENT_COLOR = 0x06,              // uint32_t filament color (rgb)
    EEVAR_RUN_SELFTEST = 0x07,                // uint8_t  selftest flag
    EEVAR_RUN_XYZCALIB = 0x08,                // uint8_t  xyz calibration flag
    EEVAR_RUN_FIRSTLAY = 0x09,                // uint8_t  first layer calibration flag
    EEVAR_FSENSOR_ENABLED = 0x0a,             // uint8_t  fsensor state
    EEVAR_ZOFFSET_DO_NOT_USE_DIRECTLY = 0x0b, // float zoffset
// use float eeprom_get_z_offset() / void eeprom_set_z_offset(float value) instead
// because EEVAR_ZOFFSET_DO_NOT_USE_DIRECTLY is unused in case sheets are enabled

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
    EEVAR_DEVHASH_IN_QR = 0x29,   // uint8_t on / off sending UID in QR
    EEVAR_FOOTER_SETTING = 0x2a,
    EEVAR_FOOTER_DRAW_TYPE = 0x2b,
    EEVAR_FAN_CHECK_ENABLED = 0x2c,   // uint8_t on / off fan check
    EEVAR_FS_AUTOLOAD_ENABLED = 0x2d, // uint8_t on / off fs autoload
    EEVAR_ODOMETER_X = 0x2e,          // float
    EEVAR_ODOMETER_Y = 0x2f,          // float
    EEVAR_ODOMETER_Z = 0x30,          // float
    EEVAR_ODOMETER_E0 = 0x31,         // float
    AXIS_STEPS_PER_UNIT_X = 0x32,     // float, used instead marlin macro DEFAULT_AXIS_STEPS_PER_UNIT
    AXIS_STEPS_PER_UNIT_Y = 0x33,     // float, used instead marlin macro DEFAULT_AXIS_STEPS_PER_UNIT
    AXIS_STEPS_PER_UNIT_Z = 0x34,     // float, used instead marlin macro DEFAULT_AXIS_STEPS_PER_UNIT
    AXIS_STEPS_PER_UNIT_E0 = 0x35,    // float, used instead marlin macro DEFAULT_AXIS_STEPS_PER_UNIT
    AXIS_MICROSTEPS_X = 0x36,         // uint16_t, used to initialize trinamic
    AXIS_MICROSTEPS_Y = 0x37,         // uint16_t, used to initialize trinamic
    AXIS_MICROSTEPS_Z = 0x38,         // uint16_t, used to initialize trinamic
    AXIS_MICROSTEPS_E0 = 0x39,        // uint16_t, used to initialize trinamic, must contain "E0" to work with marlin macros
    AXIS_RMS_CURRENT_MA_X = 0x3a,     // uint16_t, used to initialize trinamic
    AXIS_RMS_CURRENT_MA_Y = 0x3b,     // uint16_t, used to initialize trinamic
    AXIS_RMS_CURRENT_MA_Z = 0x3c,     // uint16_t, used to initialize trinamic
    AXIS_RMS_CURRENT_MA_E0 = 0x3d,    // uint16_t, used to initialize trinamic, must contain "E0" to work with marlin macros
    AXIS_Z_MAX_POS_MM = 0x3e,         // float, used in marlin Z_MAX_POS macro
    EEVAR_ODOMETER_TIME = 0x3f,       //uin32_t total print duration
    EEVAR_ACTIVE_NETDEV = 0x40,       // active network device
    EEVAR_PL_RUN = 0x41,              // active network device
    EEVAR_PL_API_KEY = 0x42,          // active network device

// wifi variables (comes under the same feature flag as eth LAN)
// FIXME: EEPROM_FEATURE_LAN probably can't be turned off, the .cpp file won't work then.
#if (EEPROM_FEATURES & EEPROM_FEATURE_LAN)
    EEVAR_WIFI_FLAG = 0x43,      // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1, lan_flag & 0b1100 -> ap_sec_t security
    EEVAR_WIFI_IP4_ADDR = 0x44,  // X.X.X.X address encoded in uint32
    EEVAR_WIFI_IP4_MSK = 0x45,   // X.X.X.X address encoded in uint32
    EEVAR_WIFI_IP4_GW = 0x46,    // X.X.X.X address encoded in uint32
    EEVAR_WIFI_IP4_DNS1 = 0x47,  // X.X.X.X address encoded in uint32
    EEVAR_WIFI_IP4_DNS2 = 0x48,  // X.X.X.X address encoded in uint32
    EEVAR_WIFI_HOSTNAME = 0x49,  // 20char string
    EEVAR_WIFI_AP_SSID = 0x4a,   // 32char string
    EEVAR_WIFI_AP_PASSWD = 0x4b, // 64char string
#endif                           // (EEPROM_FEATURES & EEPROM_FEATURE_LAN)

    EEVAR_CRC32 = 0x4d, // uint32_t crc32 for
};

enum {
    LAN_HOSTNAME_MAX_LEN = 20,
    CONNECT_TOKEN_SIZE = 20,
    PL_API_KEY_SIZE = 16,
    LAN_EEFLG_ONOFF = 1,     //EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
    LAN_EEFLG_TYPE = 2,      //EEPROM flag for user-defined settings (Switch between dhcp and static)
    WIFI_EEFLG_SEC = 0b1100, // Wifi security (ap_sec_t).
    WIFI_MAX_SSID_LEN = 32,
    WIFI_MAX_PASSWD_LEN = 64,
};

#define SelftestResult_Unknown 0
#define SelftestResult_Skipped 1
#define SelftestResult_Passed  2
#define SelftestResult_Failed  3

typedef union _SelftestResultEEprom_t {
    struct {
        uint8_t printFan : 2;     // bit 0-1
        uint8_t heatBreakFan : 2; // bit 2-3
        uint8_t xaxis : 2;        // bit 4-5
        uint8_t yaxis : 2;        // bit 6-7
        uint8_t zaxis : 2;        // bit 8-9
        uint8_t nozzle : 2;       // bit 10-11
        uint8_t bed : 2;          // bit 12-13
        uint8_t reserved0 : 2;    // bit 14-15
        uint16_t reserved1;       // bit 16-31
    };
    uint32_t ui32;
} SelftestResultEEprom_t;
//if I use uint32_t reserved : 18 for bits 14 - 31, size on 64bit system is 8, I don't know why
#ifdef __cplusplus
static_assert(sizeof(SelftestResultEEprom_t) == sizeof(uint32_t), "Incorrect SelftestResultEEprom_t size");
#endif //__cplusplus

typedef enum {
    EEPROM_INIT_Undefined = -1,
    EEPROM_INIT_Normal = 0,
    EEPROM_INIT_Defaults = 1,
    EEPROM_INIT_Upgraded = 2,
    EEPROM_INIT_in_progress = 3
} eeprom_init_status_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// initialize eeprom
/// can be called multiple times, non first call will just return status
/// cannot have function to just return static variable,
///          because this code is called before inicialization of static variables
///
/// @returns EEPROM_INIT_Normal - normal init (eeprom data valid)
///          EEPROM_INIT_Defaults - defaults loaded
///          EEPROM_INIT_Upgraded - eeprom upgraded successfully from a previous version
///          EEPROM_INIT_Undefined or EEPROM_INIT_in_progress should never be returned
extern eeprom_init_status_t eeprom_init(void);

// write default values to all variables
extern void eeprom_defaults(void);

// get variable value as variant8
extern variant8_t eeprom_get_var(enum eevar_id id);

// set variable value as variant8
extern void eeprom_set_var(enum eevar_id id, variant8_t var);

// get number of variables
extern uint8_t eeprom_get_var_count(void);

// get variable name
extern const char *eeprom_get_var_name(enum eevar_id id);

// find variables eevar_id based on its name (without the EEPROM_ prefix); return true on success
extern bool eeprom_find_var_by_name(const char *name, enum eevar_id *var_id_out);

// format variable value to string (some variables can have specific formating)
extern int eeprom_var_format(char *str, unsigned int size, enum eevar_id id, variant8_t var);

// parse variant8_t from a string previously formatted by eeprom_var_format
extern variant8_t eeprom_var_parse(enum eevar_id id, char *str);

// fill range 0x0000..0x0800 with 0xff
extern void eeprom_clear(void);

// PUT test
int8_t eeprom_test_PUT(const unsigned int);

/**
 * @brief reads Z offset, directly or from current sheet if sheets are enabled
 *
 * @return float Z offset
 */
float eeprom_get_z_offset();

/**
 * @brief writes Z offset, directly or to current sheet if sheets are enabled
 *
 * @param value value to set
 * @return true successfully set
 * @return false sheet index stored in eeprom corrupted
 */
bool eeprom_set_z_offset(float value);

// TODO sheet accesing functions should not be part of eeprom
// eeprom should not have knowledge about how sheet calibration process

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
/// MAX_SHEETS return false.
/// @param[in] index Index of the sheet profile
/// @return True when the profile can be selected, False othewise.
extern bool sheet_profile_select(uint32_t);

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
} // extern "C"
#endif //__cplusplus
