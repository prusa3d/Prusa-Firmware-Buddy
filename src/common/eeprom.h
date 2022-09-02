// eeprom.h

#pragma once

#include <stdbool.h>
#include "variant8.h"
#include "selftest_eeprom.hpp"
#include <stddef.h>

enum {
    EEPROM_ADDRESS = 0x0500, // uint16_t
    EEPROM_VERSION = 12,     // uint16_t
};

#define EEPROM_LAST_VERSION_WITH_OLD_CRC 10

#define EEPROM_FEATURE_PID_NOZ 0x0001
#define EEPROM_FEATURE_PID_BED 0x0002
#define EEPROM_FEATURE_LAN     0x0004
#define EEPROM_FEATURE_SHEETS  0x0008
#define EEPROM_FEATURE_CONNECT 0x0010
#define EEPROM_FEATURES        (EEPROM_FEATURE_PID_NOZ | EEPROM_FEATURE_PID_BED | EEPROM_FEATURE_LAN | EEPROM_FEATURE_SHEETS | EEPROM_FEATURE_CONNECT)
#define DEFAULT_HOST_NAME      "PrusaMINI"

enum {
    MAX_SHEET_NAME_LENGTH = 8,
};

// sheets must be defined even when they are not used, to be able to import data from old eeprom
typedef struct
{
    char name[MAX_SHEET_NAME_LENGTH]; //!< Can be null terminated, doesn't need to be null terminated
    float z_offset;                   //!< Z_BABYSTEP_MIN .. Z_BABYSTEP_MAX = Z_BABYSTEP_MIN*2/1000 [mm] .. Z_BABYSTEP_MAX*2/1000 [mm]
} Sheet;
enum {
    EEPROM_SHEET_SIZEOF = sizeof(Sheet)
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
    EEVAR_RUN_SELFTEST = 0x07,                // bool     selftest flag
    EEVAR_RUN_XYZCALIB = 0x08,                // bool     xyz calibration flag
    EEVAR_RUN_FIRSTLAY = 0x09,                // bool     first layer calibration flag
    EEVAR_FSENSOR_ENABLED = 0x0a,             // bool     fsensor state
    EEVAR_ZOFFSET_DO_NOT_USE_DIRECTLY = 0x0b, // float zoffset
// use float eeprom_get_z_offset() / bool eeprom_set_z_offset(float value) instead
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
    EEVAR_MENU_TIMEOUT = 0x1e, // bool on / off menu timeout flag
    EEVAR_ACTIVE_SHEET = 0x1f,
    EEVAR_SHEET_PROFILE0 = 0x20,
    EEVAR_SHEET_PROFILE1 = 0x21,
    EEVAR_SHEET_PROFILE2 = 0x22,
    EEVAR_SHEET_PROFILE3 = 0x23,
    EEVAR_SHEET_PROFILE4 = 0x24,
    EEVAR_SHEET_PROFILE5 = 0x25,
    EEVAR_SHEET_PROFILE6 = 0x26,
    EEVAR_SHEET_PROFILE_LAST = 0x27, //!< All SHEET_PROFILEs must be allocated consecutively.
    EEVAR_SELFTEST_RESULT = 0x28,    // uint32_t, two bits for each selftest part
    EEVAR_DEVHASH_IN_QR = 0x29,      // bool on / off sending UID in QR
    EEVAR_FOOTER_SETTING = 0x2a,
    EEVAR_FOOTER_DRAW_TYPE = 0x2b,
    EEVAR_FAN_CHECK_ENABLED = 0x2c,   // bool on / off fan check
    EEVAR_FS_AUTOLOAD_ENABLED = 0x2d, // bool on / off fs autoload
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
    EEVAR_ODOMETER_TIME = 0x3f,       // uin32_t total print duration
    EEVAR_ACTIVE_NETDEV = 0x40,       // active network device
    EEVAR_PL_RUN = 0x41,              // active network device
    EEVAR_PL_PASSWORD = 0x42,         // active network device

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

    EEVAR_USB_MSC_ENABLED = 0x4c, // bool, on/off

#if (EEPROM_FEATURES & EEPROM_FEATURE_CONNECT)
    EEVAR_CONNECT_HOST = 0x4d,
    EEVAR_CONNECT_TOKEN = 0x4e,
    EEVAR_CONNECT_PORT = 0x4f,
    EEVAR_CONNECT_TLS = 0x50,
    EEVAR_CONNECT_ENABLED = 0x51,
#endif

    EEVAR_JOB_ID = 0x52,            // uint16_t print job id incremented at every print start
    EEVAR_CRASH_ENABLED = 0x53,     // bool crash detection enabled
    EEVAR_CRASH_SENS_X = 0x54,      // X axis crash sensitivity
    EEVAR_CRASH_SENS_Y = 0x55,      // Y axis crash sensitivity
    EEVAR_CRASH_PERIOD_X = 0x56,    // X axis crash period (speed) threshold
    EEVAR_CRASH_PERIOD_Y = 0x57,    // Y axis crash period (speed) threshold
    EEVAR_CRASH_FILTER = 0x58,      // bool Stallguard filtration (on/off)
    EEVAR_CRASH_COUNT_X_TOT = 0x59, // number of crashes of X axis in total
    EEVAR_CRASH_COUNT_Y_TOT = 0x5a, // number of crashes of Y axis in total
    EEVAR_POWER_COUNT_TOT = 0x5b,   // number of power losses in total
    EEVAR_CRC32 = 0x5c,             // uint32_t crc32 for
};

enum {
    LAN_HOSTNAME_MAX_LEN = 20,
    CONNECT_HOST_SIZE = 20,
    CONNECT_TOKEN_SIZE = 20,
    PL_PASSWORD_SIZE = 16,
    LAN_EEFLG_ONOFF = 1,     //EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
    LAN_EEFLG_TYPE = 2,      //EEPROM flag for user-defined settings (Switch between dhcp and static)
    WIFI_EEFLG_SEC = 0b1100, // Wifi security (ap_sec_t).
    WIFI_MAX_SSID_LEN = 32,
    WIFI_MAX_PASSWD_LEN = 64,
};

typedef enum {
    EEPROM_INIT_Undefined = -1,
    EEPROM_INIT_Normal = 0,
    EEPROM_INIT_Defaults = 1,
    EEPROM_INIT_Upgraded = 2,
    EEPROM_INIT_in_progress = 3
} eeprom_init_status_t;

#ifdef __cplusplus
    #include <limits>
extern "C" {
inline constexpr size_t eeprom_num_sheets = EEVAR_SHEET_PROFILE_LAST - EEVAR_SHEET_PROFILE0 + 1;
inline constexpr float eeprom_z_offset_uncalibrated = std::numeric_limits<float>::max();
#endif //__cplusplus

/// initialize eeprom
/// can be called multiple times, non first call will just return status
/// cannot have function to just return static variable,
///          because this code is called before initialization of static variables
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

extern float eeprom_get_flt(enum eevar_id id);
extern char *eeprom_get_pch(enum eevar_id id);
extern uint8_t eeprom_get_uia(enum eevar_id id, uint8_t index);
extern uint32_t eeprom_get_ui32(enum eevar_id id);
extern int32_t eeprom_get_i32(enum eevar_id id);
extern uint16_t eeprom_get_ui16(enum eevar_id id);
extern uint8_t eeprom_get_ui8(enum eevar_id id);
extern int8_t eeprom_get_i8(enum eevar_id id);
extern bool eeprom_get_bool(enum eevar_id id);
extern Sheet eeprom_get_sheet(uint32_t index);

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

// fill old eeprom data with 0xff
extern void eeprom_clear(void);

// PUT test
int8_t eeprom_test_PUT(const unsigned int);

#ifdef __cplusplus
} // extern "C"
#endif //__cplusplus
