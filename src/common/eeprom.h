// eeprom.h

#pragma once

#include <stdbool.h>
#include "variant8.h"
#include "eeprom_function_api.h"
#include <stddef.h>

enum {
    EEPROM_ADDRESS = 0x0500, // uint16_t
    EEPROM_VERSION = 22,     // uint16_t
};

#define EEPROM_LAST_VERSION_WITH_OLD_CRC 10

#define EEPROM_FEATURE_PID_NOZ  0x0001
#define EEPROM_FEATURE_PID_BED  0x0002
#define EEPROM_FEATURE_LAN      0x0004
#define EEPROM_FEATURE_SHEETS   0x0008
#define EEPROM_FEATURE_LOADCELL 0x0010
#define EEPROM_FEATURE_MMU2     0x0020
#define EEPROM_FEATURE_CONNECT  0x0040

#define EEPROM_FEATURES   (EEPROM_FEATURE_PID_NOZ | EEPROM_FEATURE_PID_BED | EEPROM_FEATURE_LAN | EEPROM_FEATURE_LOADCELL | EEPROM_FEATURE_SHEETS | EEPROM_FEATURE_MMU2 | EEPROM_FEATURE_CONNECT)
#define DEFAULT_HOST_NAME LAN_HOSTNAME_DEF

#define EEPROM_MAX_TOOL_COUNT 6

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

#pragma pack(push, 1)

typedef struct {
    uint8_t type;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint16_t period;
    uint8_t color_count; // number of colors extra, not counting base color inside the model
    uint8_t next_index;
} Animation_model;

typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t next_index;

} Color_model;

typedef struct {
    float x;
    float y;
} KennelPosition;

typedef struct {
    float x;
    float y;
    float z;
} ToolOffset;

/**
 * @brief Generic selftest results.
 */
typedef enum {
    TestResult_Unknown = 0,
    TestResult_Skipped = 1,
    TestResult_Passed = 2,
    TestResult_Failed = 3,
} TestResult;

/**
 * @brief Selftest results for a network interface.
 */
typedef enum {
    TestResultNet_Unknown = 0,   // test did not run
    TestResultNet_Unlinked = 1,  // wifi not present, eth cable unplugged
    TestResultNet_Down = 2,      // wifi present, eth cable plugged, not selected in lan settings
    TestResultNet_NoAddress = 3, // wifi present, no address obtained from DHCP
    TestResultNet_Up = 4,        // wifi present, eth cable plugged, selected in lan settings
} TestResultNet;

/**
 * @brief Results for selftests of one tool.
 */
typedef struct {
    TestResult printFan : 2;
    TestResult heatBreakFan : 2;
    TestResult nozzle : 2;
    TestResult fsensor : 2;
    TestResult loadcell : 2;
    TestResult sideFsensor : 2;
    TestResult kenneloffset : 2;
    TestResult tooloffset : 2;
} SelftestTool;

/**
 * @brief Test results compacted in eeprom.
 */
typedef struct {
    TestResult xaxis : 2;
    TestResult yaxis : 2;
    TestResult zaxis : 2;
    TestResult bed : 2;
    TestResultNet eth : 3;
    TestResultNet wifi : 3;
    TestResult zalign : 2;
    SelftestTool tools[EEPROM_MAX_TOOL_COUNT];
} SelftestResult;

#pragma pack(pop)

enum eevar_id {
    // basic variables
    EEVAR_VERSION = 0x00,                     // uint16_t eeprom version
    EEVAR_FEATURES = 0x01,                    // uint16_t feature mask
    EEVAR_DATASIZE = 0x02,                    // uint16_t eeprom data size
    EEVAR_FW_VERSION = 0x03,                  // uint16_t encoded firmware version (e.g. 403 for 4.0.3)
    EEVAR_FW_BUILD = 0x04,                    // uint16_t firmware build number
    EEVAR_FILAMENT_TYPE = 0x05,               // uint8_t  filament type in the first extruder
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
    EEVAR_SELFTEST_RESULT_V1 = 0x28, //!< Not used, was EEVAR_SELFTEST_RESULT, replaced by EEVAR_SELFTEST_RESULT_V2
    EEVAR_DEVHASH_IN_QR = 0x29,      // bool on / off sending UID in QR
    EEVAR_FOOTER_SETTING = 0x2a,
    EEVAR_FOOTER_DRAW_TYPE = 0x2b,
    EEVAR_FAN_CHECK_ENABLED = 0x2c,   // bool on / off fan check
    EEVAR_FS_AUTOLOAD_ENABLED = 0x2d, // bool on / off fs autoload
    EEVAR_ODOMETER_X = 0x2e,          // float
    EEVAR_ODOMETER_Y = 0x2f,          // float
    EEVAR_ODOMETER_Z = 0x30,          // float
    EEVAR_ODOMETER_E0 = 0x31,         // float, filament passed through extruder 0
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
    EEVAR_WIFI_FLAG = 0x43,      // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1, lan_flag & 0b1100 -> reserved, previously ap_sec_t security
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

    EEVAR_JOB_ID = 0x52,             // uint16_t print job id incremented at every print start
    EEVAR_CRASH_ENABLED = 0x53,      // bool crash detection enabled
    EEVAR_CRASH_SENS_X = 0x54,       // X axis crash sensitivity
    EEVAR_CRASH_SENS_Y = 0x55,       // Y axis crash sensitivity
    EEVAR_CRASH_MAX_PERIOD_X = 0x56, // X axis max crash period (speed) threshold
    EEVAR_CRASH_MAX_PERIOD_Y = 0x57, // Y axis max crash period (speed) threshold
    EEVAR_CRASH_FILTER = 0x58,       // bool Stallguard filtration (on/off)
    EEVAR_CRASH_COUNT_X_TOT = 0x59,  // number of crashes of X axis in total
    EEVAR_CRASH_COUNT_Y_TOT = 0x5a,  // number of crashes of Y axis in total
    EEVAR_POWER_COUNT_TOT = 0x5b,    // number of power losses in total

    // private variables
    EEVAR_TIME_FORMAT = 0x5c, // uint8_t time format (12h(PM/AM)/24h)
#if (EEPROM_FEATURES & EEPROM_FEATURE_LOADCELL)
    EEVAR_LOADCELL_SCALE = 0x5d,
    EEVAR_LOADCELL_THRS_STATIC = 0x5e,
    EEVAR_LOADCELL_HYST = 0x5f,
    EEVAR_LOADCELL_THRS_CONTINOUS = 0x60,
#endif
    EEVAR_FS_REF_VALUE_0 = 0x61,  // int32_t value of filament sensor in moment of calibration, it can be either with or w/o filament present
    EEVAR_FS_VALUE_SPAN_0 = 0x62, // uint32_t minimal difference of raw values between the two states of the filament sensor
    EEVAR_FS_REF_VALUE_1 = 0x63,  // int32_t value of filament sensor in moment of calibration, it can be either with or w/o filament present
    EEVAR_FS_VALUE_SPAN_1 = 0x64, // uint32_t minimal difference of raw values between the two states of the filament sensor
    EEVAR_FS_REF_VALUE_2 = 0x65,  // int32_t value of filament sensor in moment of calibration, it can be either with or w/o filament present
    EEVAR_FS_VALUE_SPAN_2 = 0x66, // uint32_t minimal difference of raw values between the two states of the filament sensor
    EEVAR_FS_REF_VALUE_3 = 0x67,  // int32_t value of filament sensor in moment of calibration, it can be either with or w/o filament present
    EEVAR_FS_VALUE_SPAN_3 = 0x68, // uint32_t minimal difference of raw values between the two states of the filament sensor
    EEVAR_FS_REF_VALUE_4 = 0x69,  // int32_t value of filament sensor in moment of calibration, it can be either with or w/o filament present
    EEVAR_FS_VALUE_SPAN_4 = 0x6A, // uint32_t minimal difference of raw values between the two states of the filament sensor
    EEVAR_FS_REF_VALUE_5 = 0x6B,  // int32_t value of filament sensor in moment of calibration, it can be either with or w/o filament present
    EEVAR_FS_VALUE_SPAN_5 = 0x6C, // uint32_t minimal difference of raw values between the two states of the filament sensor
    EEVAR_SIDE_FS_REF_VALUE_0 = 0x6D,
    EEVAR_SIDE_FS_VALUE_SPAN_0 = 0x6E,
    EEVAR_SIDE_FS_REF_VALUE_1 = 0x6F,
    EEVAR_SIDE_FS_VALUE_SPAN_1 = 0x70,
    EEVAR_SIDE_FS_REF_VALUE_2 = 0x71,
    EEVAR_SIDE_FS_VALUE_SPAN_2 = 0x72,
    EEVAR_SIDE_FS_REF_VALUE_3 = 0x73,
    EEVAR_SIDE_FS_VALUE_SPAN_3 = 0x74,
    EEVAR_SIDE_FS_REF_VALUE_4 = 0x75,
    EEVAR_SIDE_FS_VALUE_SPAN_4 = 0x76,
    EEVAR_SIDE_FS_REF_VALUE_5 = 0x77,
    EEVAR_SIDE_FS_VALUE_SPAN_5 = 0x78,
    EEVAR_PRINT_PROGRESS_TIME = 0x79,   // uint16 screen progress time in seconds
    EEVAR_TMC_WAVETABLE_ENABLED = 0x7A, // bool enable wavetable in TMC drivers
    EEVAR_MMU2_ENABLED = 0x7B,          // enable usage of MMU2
    EEVAR_MMU2_CUTTER = 0x7C,           // use MMU2 cutter when it sees fit
    EEVAR_MMU2_STEALTH_MODE = 0x7D,     // run MMU2 in stealth mode wherever possible
    EEVAR_RUN_LEDS = 0x7E,              // run animations on leds
    EEVAR_ANIMATION_IDLE = 0x7F,
    EEVAR_ANIMATION_PRINTING = 0x80,
    EEVAR_ANIMATION_PAUSING = 0x81,
    EEVAR_ANIMATION_RESUMING = 0x82,
    EEVAR_ANIMATION_ABORTING = 0x83,
    EEVAR_ANIMATION_FINISHING = 0x84,
    EEVAR_ANIMATION_WARNING = 0x85,
    EEVAR_ANIMATION_POWER_PANIC = 0x86,
    EEVAR_ANIMATION_POWER_UP = 0x87,
    EEVAR_ANIMATION_COLOR0 = 0x88,
    EEVAR_ANIMATION_COLOR1 = 0x89,
    EEVAR_ANIMATION_COLOR2 = 0x8A,
    EEVAR_ANIMATION_COLOR3 = 0x8B,
    EEVAR_ANIMATION_COLOR4 = 0x8C,
    EEVAR_ANIMATION_COLOR5 = 0x8D,
    EEVAR_ANIMATION_COLOR6 = 0x8E,
    EEVAR_ANIMATION_COLOR7 = 0x8F,
    EEVAR_ANIMATION_COLOR8 = 0x90,
    EEVAR_ANIMATION_COLOR9 = 0x91,
    EEVAR_ANIMATION_COLOR10 = 0x92,
    EEVAR_ANIMATION_COLOR11 = 0x93,
    EEVAR_ANIMATION_COLOR12 = 0x94,
    EEVAR_ANIMATION_COLOR13 = 0x95,
    EEVAR_ANIMATION_COLOR14 = 0x96,
    EEVAR_ANIMATION_COLOR_LAST = 0x97, //< All colors must be allocated consecutively
    EEVAR_HEAT_ENTIRE_BED = 0x98,
    EEVAR_TOUCH_ENABLED = 0x99,
    EEVAR_KENNEL_POSITION_0 = 0x9A,
    EEVAR_KENNEL_POSITION_1 = 0x9B,
    EEVAR_KENNEL_POSITION_2 = 0x9C,
    EEVAR_KENNEL_POSITION_3 = 0x9D,
    EEVAR_KENNEL_POSITION_4 = 0x9E,
    EEVAR_KENNEL_POSITION_5 = 0x9F,
    EEVAR_TOOL_OFFSET_0 = 0xA0,
    EEVAR_TOOL_OFFSET_1 = 0xA1,
    EEVAR_TOOL_OFFSET_2 = 0xA2,
    EEVAR_TOOL_OFFSET_3 = 0xA3,
    EEVAR_TOOL_OFFSET_4 = 0xA4,
    EEVAR_TOOL_OFFSET_5 = 0xA5,
    EEVAR_FILAMENT_TYPE_1 = 0xA6, // uint8_t  filament type in >2nd< extruder, the extruder is stored in the original EEVAR_FILAMENT_TYPE
    EEVAR_FILAMENT_TYPE_2 = 0xA7, // uint8_t  filament type in 3rd extruder
    EEVAR_FILAMENT_TYPE_3 = 0xA8, // uint8_t  filament type in 4th extruder
    EEVAR_FILAMENT_TYPE_4 = 0xA9, // uint8_t  filament type in 5th extruder
    EEVAR_FILAMENT_TYPE_5 = 0xAA, // uint8_t  filament type in 6th extruder
    EEVAR_HEATUP_BED = 0xAB,
    EEVAR_NOZZLE_DIA_0 = 0xAC, // float
    EEVAR_NOZZLE_DIA_1 = 0xAD, // float
    EEVAR_NOZZLE_DIA_2 = 0xAE, // float
    EEVAR_NOZZLE_DIA_3 = 0xAF, // float
    EEVAR_NOZZLE_DIA_4 = 0xB0, // float
    EEVAR_NOZZLE_DIA_5 = 0xB1, // float
    EEVAR_HWCHECK_NOZZLE = 0xB2,
    EEVAR_HWCHECK_MODEL = 0xB3,
    EEVAR_HWCHECK_FIRMW = 0xB4,
    EEVAR_HWCHECK_GCODE = 0xB5,
    EEVAR_SELFTEST_RESULT_V2 = 0xB6, // Wider selftest results made for XL
    EEVAR_HOMING_BDIVISOR_X = 0xB7,
    EEVAR_HOMING_BDIVISOR_Y = 0xB8,
    EEVAR_ENABLE_SIDE_LEDS = 0xB9,      // bool side led on/off
    EEVAR_ODOMETER_E1 = 0xBA,           // float, filament passed through extruder 1
    EEVAR_ODOMETER_E2 = 0xBB,           // float, filament passed through extruder 2
    EEVAR_ODOMETER_E3 = 0xBC,           // float, filament passed through extruder 3
    EEVAR_ODOMETER_E4 = 0xBD,           // float, filament passed through extruder 4
    EEVAR_ODOMETER_E5 = 0xBE,           // float, filament passed through extruder 5
    EEVAR_ODOMETER_T0 = 0xBF,           // uint32_t, tool 0 pick counter
    EEVAR_ODOMETER_T1 = 0xC0,           // uint32_t, tool 1 pick counter
    EEVAR_ODOMETER_T2 = 0xC1,           // uint32_t, tool 2 pick counter
    EEVAR_ODOMETER_T3 = 0xC2,           // uint32_t, tool 3 pick counter
    EEVAR_ODOMETER_T4 = 0xC3,           // uint32_t, tool 4 pick counter
    EEVAR_ODOMETER_T5 = 0xC4,           // uint32_t, tool 5 pick counter
    EEVAR_HWCHECK_COMPATIBILITY = 0xC5, // uint8_t

    EEVAR_CRC32 // uint32_t crc32 for
};

enum {
    LAN_HOSTNAME_MAX_LEN = 20,
    CONNECT_HOST_SIZE = 20,
    CONNECT_TOKEN_SIZE = 20,
    LAN_EEFLG_ONOFF = 1, // EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
    LAN_EEFLG_TYPE = 2,  // EEPROM flag for user-defined settings (Switch between dhcp and static)
    PL_PASSWORD_SIZE = 16,
    WIFI_EEFLG_SEC = 0b1100, // reserved, previously Wifi security (ap_sec_t).
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
inline constexpr size_t eeprom_num_colors = EEVAR_ANIMATION_COLOR_LAST - EEVAR_ANIMATION_COLOR0 + 1;
#endif //__cplusplus

extern size_t eeprom_init_crc_error();
extern size_t eeprom_init_upgraded();
extern size_t eeprom_init_upgrade_failed();

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

extern void eeprom_set_i8(enum eevar_id id, int8_t i8);
extern void eeprom_set_bool(enum eevar_id id, bool b);
extern void eeprom_set_ui8(enum eevar_id id, uint8_t ui8);
extern void eeprom_set_i16(enum eevar_id id, int16_t i16);
extern void eeprom_set_ui16(enum eevar_id id, uint16_t ui16);
extern void eeprom_set_i32(enum eevar_id id, int32_t i32);
extern void eeprom_set_ui32(enum eevar_id id, uint32_t ui32);
extern void eeprom_set_flt(enum eevar_id id, float flt);
extern void eeprom_set_pchar(enum eevar_id id, char *pch, uint16_t count, int init);
extern bool eeprom_set_sheet(uint32_t index, Sheet sheet);
extern KennelPosition eeprom_get_kennel_position(int kennel_idx);
extern void eeprom_set_kennel_position(int kennel_idx, KennelPosition position);
extern ToolOffset eeprom_get_tool_offset(int tool_idx);
extern void eeprom_set_tool_offset(int tool_idx, ToolOffset offset);

/**
 * @brief Load nozzle diameter from EEPROM.
 * @param tool_idx for this tool, indexed from 0
 * @return nozzle diameter [mm]
 */
float eeprom_get_nozzle_dia(int tool_idx);

/**
 * @brief Store nozzle diameter to EEPROM.
 * @param tool_idx for this tool, indexed from 0
 * @param diameter nozzle diameter [mm]
 */
void eeprom_set_nozzle_dia(int tool_idx, float diameter);

/**
 * @brief Get selftest results for tool.
 * @param results load these results
 */
extern void eeprom_get_selftest_results(SelftestResult *results);

/**
 * @brief Set selftest results.
 * @param results store these results
 */
extern void eeprom_set_selftest_results(const SelftestResult *results);

/**
 * @brief Get selftest results for tool.
 * @param tool_idx get only this tool
 * @param results load these tool results
 */
extern void eeprom_get_selftest_results_tool(int tool_idx, SelftestTool *tool);

/**
 * @brief Set selftest results for tool.
 * @param tool_idx set only this tool
 * @param results store these tool results
 */
extern void eeprom_set_selftest_results_tool(int tool_idx, const SelftestTool *tool);

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

#ifdef __cplusplus
} // extern "C"
#endif //__cplusplus

#ifdef __cplusplus
    #include "led_animations/led_types.h"
    #include <array>
extern Animation_model eeprom_get_animation(uint32_t index, std::array<leds::Color, eeprom_num_colors> &colors);
extern bool eeprom_set_animation(uint32_t index, Animation_model animation, std::array<leds::Color, eeprom_num_colors> colors);
#endif //__cplusplus
