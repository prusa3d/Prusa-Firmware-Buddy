// eeprom.cpp

#include <string.h>
#include <float.h>

#include "eeprom.h"
#include "eeprom_function_api.h"
#include "st25dv64k.h"
#include "dbg.h"
#include "cmsis_os.h"
#include "crc32.h"
#include "version.h"
#include "wdt.h"
#include "../Marlin/src/module/temperature.h"
#include "../include/marlin/Configuration.h"
#include "../include/marlin/Configuration_adv.h"
#include "cmath_ext.h"
#include "footer_eeprom.hpp"
#include <bitset>

static const constexpr uint8_t EEPROM__PADDING = 4;
static const constexpr uint8_t EEPROM_MAX_NAME = 16;               // maximum name length (with '\0')
static const constexpr uint16_t EEPROM_MAX_DATASIZE = 512;         // maximum datasize
static const constexpr uint16_t EEPROM_FIRST_VERSION_CRC = 0x0004; // first eeprom version with crc support

// flags will be used also for selective variable reset default values in some cases (shipping etc.))
static const constexpr uint16_t EEVAR_FLG_READONLY = 0x0001; // variable is read only

// measure time needed to update crc
//#define EEPROM_MEASURE_CRC_TIME

#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
typedef struct
{
    char name[MAX_SHEET_NAME_LENGTH]; //!< Can be null terminated, doesn't need to be null terminated
    float z_offset;                   //!< Z_BABYSTEP_MIN .. Z_BABYSTEP_MAX = Z_BABYSTEP_MIN*2/1000 [mm] .. Z_BABYSTEP_MAX*2/1000 [mm]
} Sheet;

enum {
    MAX_SHEETS = 8,
    EEPROM_SHEET_SIZEOF = sizeof(Sheet)
};

#endif
// this pragma pack must remain intact, the ordering of EEPROM variables is not alignment-friendly
#pragma pack(push, 1)

// eeprom map entry structure
typedef struct _eeprom_entry_t {
    const char name[EEPROM_MAX_NAME];
    uint8_t type;   // variant8 data type
    uint8_t count;  // number of elements
    uint16_t flags; // flags
} eeprom_entry_t;

// eeprom vars structure (used for defaults)
typedef struct _eeprom_vars_t {
    uint16_t VERSION;
    uint16_t FEATURES;
    uint16_t DATASIZE;
    uint16_t FWVERSION;
    uint16_t FWBUILD;
    uint8_t FILAMENT_TYPE;
    uint32_t FILAMENT_COLOR;
    uint8_t RUN_SELFTEST;
    uint8_t RUN_XYZCALIB;
    uint8_t RUN_FIRSTLAY;
    uint8_t FSENSOR_ENABLED;
    float ZOFFSET;
    float PID_NOZ_P;
    float PID_NOZ_I;
    float PID_NOZ_D;
    float PID_BED_P;
    float PID_BED_I;
    float PID_BED_D;
    uint8_t LAN_FLAG;
    uint32_t LAN_IP4_ADDR;
    uint32_t LAN_IP4_MSK;
    uint32_t LAN_IP4_GW;
    uint32_t LAN_IP4_DNS1;
    uint32_t LAN_IP4_DNS2;
    char LAN_HOSTNAME[LAN_HOSTNAME_MAX_LEN + 1];
    int8_t TIMEZONE;
    uint8_t SOUND_MODE;
    uint8_t SOUND_VOLUME;
    uint16_t LANGUAGE;
    uint8_t FILE_SORT;
    uint8_t MENU_TIMEOUT;
    uint8_t ACTIVE_SHEET;
    Sheet SHEET_PROFILE0;
    Sheet SHEET_PROFILE1;
    Sheet SHEET_PROFILE2;
    Sheet SHEET_PROFILE3;
    Sheet SHEET_PROFILE4;
    Sheet SHEET_PROFILE5;
    Sheet SHEET_PROFILE6;
    Sheet SHEET_PROFILE7;
    uint32_t SELFTEST_RESULT;
    uint8_t DEVHASH_IN_QR;
    uint32_t FOOTER_SETTING;
    uint32_t FOOTER_DRAW_TYPE;
    uint8_t FAN_CHECK_ENABLED;
    uint8_t FS_AUTOLOAD_ENABLED;
    float EEVAR_ODOMETER_X;
    float EEVAR_ODOMETER_Y;
    float EEVAR_ODOMETER_Z;
    float EEVAR_ODOMETER_E0;
    float AXIS_STEPS_PER_UNIT_X;
    float AXIS_STEPS_PER_UNIT_Y;
    float AXIS_STEPS_PER_UNIT_Z;
    float AXIS_STEPS_PER_UNIT_E0;
    uint16_t AXIS_MICROSTEPS_X;
    uint16_t AXIS_MICROSTEPS_Y;
    uint16_t AXIS_MICROSTEPS_Z;
    uint16_t AXIS_MICROSTEPS_E0;
    uint16_t AXIS_RMS_CURRENT_MA_X;
    uint16_t AXIS_RMS_CURRENT_MA_Y;
    uint16_t AXIS_RMS_CURRENT_MA_Z;
    uint16_t AXIS_RMS_CURRENT_MA_E0;
    float AXIS_Z_MAX_POS_MM;
    uint32_t ODOMETER_TIME;
    char _PADDING[EEPROM__PADDING];
    uint32_t CRC32;
} eeprom_vars_t;

static_assert(sizeof(eeprom_vars_t) % 4 == 0, "EEPROM__PADDING needs to be adjusted so CRC32 could work.");
#pragma pack(pop)

// clang-format off

// eeprom map
static const eeprom_entry_t eeprom_map[] = {
    { "VERSION",         VARIANT8_UI16,  1, EEVAR_FLG_READONLY }, // EEVAR_VERSION
    { "FEATURES",        VARIANT8_UI16,  1, EEVAR_FLG_READONLY }, // EEVAR_FEATURES
    { "DATASIZE",        VARIANT8_UI16,  1, EEVAR_FLG_READONLY }, // EEVAR_DATASIZE
    { "FW_VERSION",      VARIANT8_UI16,  1, 0 }, // EEVAR_FW_VERSION
    { "FW_BUILD",        VARIANT8_UI16,  1, 0 }, // EEVAR_FW_BUILD
    { "FILAMENT_TYPE",   VARIANT8_UI8,   1, 0 }, // EEVAR_FILAMENT_TYPE
    { "FILAMENT_COLOR",  VARIANT8_UI32,  1, 0 }, // EEVAR_FILAMENT_COLOR
    { "RUN_SELFTEST",    VARIANT8_UI8,   1, 0 }, // EEVAR_RUN_SELFTEST
    { "RUN_XYZCALIB",    VARIANT8_UI8,   1, 0 }, // EEVAR_RUN_XYZCALIB
    { "RUN_FIRSTLAY",    VARIANT8_UI8,   1, 0 }, // EEVAR_RUN_FIRSTLAY
    { "FSENSOR_ENABLED", VARIANT8_UI8,   1, 0 }, // EEVAR_FSENSOR_ENABLED
    { "ZOFFSET",         VARIANT8_FLT,   1, 0 }, // EEVAR_ZOFFSET
    { "PID_NOZ_P",       VARIANT8_FLT,   1, 0 }, // EEVAR_PID_NOZ_P
    { "PID_NOZ_I",       VARIANT8_FLT,   1, 0 }, // EEVAR_PID_NOZ_I
    { "PID_NOZ_D",       VARIANT8_FLT,   1, 0 }, // EEVAR_PID_NOZ_D
    { "PID_BED_P",       VARIANT8_FLT,   1, 0 }, // EEVAR_PID_BED_P
    { "PID_BED_I",       VARIANT8_FLT,   1, 0 }, // EEVAR_PID_BED_I
    { "PID_BED_D",       VARIANT8_FLT,   1, 0 }, // EEVAR_PID_BED_D
    { "LAN_FLAG",        VARIANT8_UI8,   1, 0 }, // EEVAR_LAN_FLAG
    { "LAN_IP4_ADDR",    VARIANT8_UI32,  1, 0 }, // EEVAR_LAN_IP4_ADDR
    { "LAN_IP4_MSK",     VARIANT8_UI32,  1, 0 }, // EEVAR_LAN_IP4_MSK
    { "LAN_IP4_GW",      VARIANT8_UI32,  1, 0 }, // EEVAR_LAN_IP4_GW
    { "LAN_IP4_DNS1",    VARIANT8_UI32,  1, 0 }, // EEVAR_LAN_IP4_DNS1
    { "LAN_IP4_DNS2",    VARIANT8_UI32,  1, 0 }, // EEVAR_LAN_IP4_DNS2
    { "LAN_HOSTNAME",    VARIANT8_PCHAR, LAN_HOSTNAME_MAX_LEN + 1, 0 }, // EEVAR_LAN_HOSTNAME
    { "TIMEZONE",        VARIANT8_I8,    1, 0 }, // EEVAR_TIMEZONE
    { "SOUND_MODE",      VARIANT8_UI8,   1, 0 }, // EEVAR_SOUND_MODE
    { "SOUND_VOLUME",    VARIANT8_UI8,   1, 0 }, // EEVAR_SOUND_VOLUME
    { "LANGUAGE",        VARIANT8_UI16,  1, 0 }, // EEVAR_LANGUAGE
    { "FILE_SORT",       VARIANT8_UI8,   1, 0 }, // EEVAR_FILE_SORT
    { "MENU_TIMEOUT",    VARIANT8_UI8,   1, 0 }, // EEVAR_MENU_TIMEOUT
    { "ACTIVE_SHEET",    VARIANT8_UI8,   1, 0 }, // EEVAR_ACTIVE_SHEET
    { "SHEET_PROFILE0",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SHEET_PROFILE1",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SHEET_PROFILE2",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SHEET_PROFILE3",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SHEET_PROFILE4",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SHEET_PROFILE5",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SHEET_PROFILE6",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SHEET_PROFILE7",  VARIANT8_PUI8,  sizeof(Sheet), 0 },
    { "SELFTEST_RESULT", VARIANT8_UI32,  1, 0 }, // EEVAR_SELFTEST_RESULT
    { "DEVHASH_IN_QR",   VARIANT8_UI8,   1, 0 }, // EEVAR_DEVHASH_IN_QR
    { "FOOTER_SETTING",  VARIANT8_UI32,  1, 0 }, // EEVAR_FOOTER_SETTING
    { "FOOTER_DRAW_TP"  ,VARIANT8_UI32,  1, 0 }, // EEVAR_FOOTER_DRAW_TYPE
    { "FAN_CHECK_ENA",   VARIANT8_UI8,   1, 0 }, // EEVAR_FAN_CHECK_ENABLED
    { "FS_AUTOL_ENA",    VARIANT8_UI8,   1, 0},  // EEVAR_FS_AUTOLOAD_ENABLED
    { "ODOMETER_X",      VARIANT8_FLT,   1, 0 }, // EEVAR_ODOMETER_X
    { "ODOMETER_Y",      VARIANT8_FLT,   1, 0 }, // EEVAR_ODOMETER_Y
    { "ODOMETER_Z",      VARIANT8_FLT,   1, 0 }, // EEVAR_ODOMETER_Z
    { "ODOMETER_E",      VARIANT8_FLT,   1, 0 }, // EEVAR_ODOMETER_E0
    { "STEPS_PR_UNIT_X", VARIANT8_FLT,   1, 0 }, // AXIS_STEPS_PER_UNIT_X
    { "STEPS_PR_UNIT_Y", VARIANT8_FLT,   1, 0 }, // AXIS_STEPS_PER_UNIT_Y
    { "STEPS_PR_UNIT_Z", VARIANT8_FLT,   1, 0 }, // AXIS_STEPS_PER_UNIT_Z
    { "STEPS_PR_UNIT_E", VARIANT8_FLT,   1, 0 }, // AXIS_STEPS_PER_UNIT_E0
    { "MICROSTEPS_X",    VARIANT8_UI16,  1, 0 }, // AXIS_MICROSTEPS_X
    { "MICROSTEPS_Y",    VARIANT8_UI16,  1, 0 }, // AXIS_MICROSTEPS_Y
    { "MICROSTEPS_Z",    VARIANT8_UI16,  1, 0 }, // AXIS_MICROSTEPS_Z
    { "MICROSTEPS_E",    VARIANT8_UI16,  1, 0 }, // AXIS_MICROSTEPS_E0
    { "RMS_CURR_MA_X",   VARIANT8_UI16,  1, 0 }, // AXIS_RMS_CURRENT_MA_X
    { "RMS_CURR_MA_Y",   VARIANT8_UI16,  1, 0 }, // AXIS_RMS_CURRENT_MA_Y
    { "RMS_CURR_MA_Z",   VARIANT8_UI16,  1, 0 }, // AXIS_RMS_CURRENT_MA_Z
    { "RMS_CURR_MA_E",   VARIANT8_UI16,  1, 0 }, // AXIS_RMS_CURRENT_MA_E0
    { "Z_MAX_POS_MM",    VARIANT8_FLT,   1, 0 }, // AXIS_Z_MAX_POS_MM
    { "ODOMETER_TIME",   VARIANT8_UI32,  1, 0 }, // EEVAR_LAN_ODOMETER_TIME
    { "_PADDING",        VARIANT8_PCHAR, EEPROM__PADDING, 0 }, // EEVAR__PADDING32
    { "CRC32",           VARIANT8_UI32,  1, 0 }, // EEVAR_CRC32
};

static const constexpr uint32_t EEPROM_VARCOUNT = sizeof(eeprom_map) / sizeof(eeprom_entry_t);
static const constexpr uint32_t EEPROM_DATASIZE = sizeof(eeprom_vars_t);

static constexpr float default_axis_steps_flt[4] = DEFAULT_AXIS_STEPS_PER_UNIT; //to be able to access macro values
static constexpr int default_axis_steps_int[4] = DEFAULT_AXIS_STEPS_PER_UNIT; //to be able to access macro values

// eeprom variable defaults
static const eeprom_vars_t eeprom_var_defaults = {
    EEPROM_VERSION,  // EEVAR_VERSION
    EEPROM_FEATURES, // EEVAR_FEATURES
    EEPROM_DATASIZE, // EEVAR_DATASIZE
    0,               // EEVAR_FW_VERSION
    0,               // EEVAR_FW_BUILD
    0,               // EEVAR_FILAMENT_TYPE
    0,               // EEVAR_FILAMENT_COLOR
    1,               // EEVAR_RUN_SELFTEST
    1,               // EEVAR_RUN_XYZCALIB
    1,               // EEVAR_RUN_FIRSTLAY
    1,               // EEVAR_FSENSOR_ENABLED
    0,               // EEVAR_ZOFFSET
#if ENABLED(PIDTEMP)
    DEFAULT_Kp,      // EEVAR_PID_NOZ_P
    scalePID_i(DEFAULT_Ki),      // EEVAR_PID_NOZ_I
    scalePID_d(DEFAULT_Kd),      // EEVAR_PID_NOZ_D
#else
    0, 0, 0,
#endif
    DEFAULT_bedKp,   // EEVAR_PID_BED_P
    scalePID_i(DEFAULT_bedKi),   // EEVAR_PID_BED_I
    scalePID_d(DEFAULT_bedKd),   // EEVAR_PID_BED_D
    0,               // EEVAR_LAN_FLAG
    0,               // EEVAR_LAN_IP4_ADDR
    0,               // EEVAR_LAN_IP4_MSK
    0,               // EEVAR_LAN_IP4_GW
    0,               // EEVAR_LAN_IP4_DNS1
    0,               // EEVAR_LAN_IP4_DNS2
    "PrusaMINI",     // EEVAR_LAN_HOSTNAME
    0,               // EEVAR_TIMEZONE
    0xff,            // EEVAR_SOUND_MODE
    5,               // EEVAR_SOUND_VOLUME
    0xffff,          // EEVAR_LANGUAGE
    0,               // EEVAR_FILE_SORT
    1,               // EEVAR_MENU_TIMEOUT
    0,               // EEVAR_ACTIVE_SHEET
    {"Smooth1", 0.0f },
    {"Smooth2", FLT_MAX },
    {"Textur1", FLT_MAX },
    {"Textur2", FLT_MAX },
    {"Custom1", FLT_MAX },
    {"Custom2", FLT_MAX },
    {"Custom3", FLT_MAX },
    {"Custom4", FLT_MAX },
    0,               // EEVAR_SELFTEST_RESULT
    1,               // EEVAR_DEVHASH_IN_QR
    footer::eeprom::Encode(footer::DefaultItems), // EEVAR_FOOTER_SETTING
    uint32_t(footer::ItemDrawCnf::Default()), // EEVAR_FOOTER_DRAW_TYPE
    1,               // EEVAR_FAN_CHECK_ENABLED
    0,               // EEVAR_FS_AUTOLOAD_ENABLED
    0,               // EEVAR_ODOMETER_X
    0,               // EEVAR_ODOMETER_Y
    0,               // EEVAR_ODOMETER_Z
    0,               // EEVAR_ODOMETER_E0
    default_axis_steps_flt[0] * ((DEFAULT_INVERT_X_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_X
    default_axis_steps_flt[1] * ((DEFAULT_INVERT_Y_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_Y
    default_axis_steps_flt[2] * ((DEFAULT_INVERT_Z_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_Z
    default_axis_steps_flt[3] * ((DEFAULT_INVERT_E0_DIR == true) ? -1.f : 1.f),  // AXIS_STEPS_PER_UNIT_E0
    X_MICROSTEPS,           // AXIS_MICROSTEPS_X
    Y_MICROSTEPS,           // AXIS_MICROSTEPS_Y
    Z_MICROSTEPS,           // AXIS_MICROSTEPS_Z
    E0_MICROSTEPS,          // AXIS_MICROSTEPS_E0
    X_CURRENT,              // AXIS_RMS_CURRENT_MA_X
    Y_CURRENT,              // AXIS_RMS_CURRENT_MA_Y
    Z_CURRENT,              // AXIS_RMS_CURRENT_MA_Z
    E0_CURRENT,             // AXIS_RMS_CURRENT_MA_E0
    DEFAULT_Z_MAX_POS,      // AXIS_Z_MAX_POS_MM
    0,               // EEVAR_ODOMETER_TIME
    "",                     // EEVAR__PADDING
    0xffffffff,             // EEVAR_CRC32
};
// clang-format on

// semaphore handle (lock/unlock)
// zero initialized variable is fine even during initialization called from startup script
static osSemaphoreId eeprom_sema = 0;

static inline void eeprom_lock(void) {
    osSemaphoreWait(eeprom_sema, osWaitForever);
}

static inline void eeprom_unlock(void) {
    osSemaphoreRelease(eeprom_sema);
}

// forward declarations of private functions

static uint16_t eeprom_var_size(uint8_t id);
static uint16_t eeprom_var_addr(uint8_t id);
//static void eeprom_print_vars(void);
static int eeprom_convert_from_v2(void);
static int eeprom_convert_from(uint16_t version, uint16_t features);

static int eeprom_check_crc32(void);
static void eeprom_update_crc32();

static uint16_t eeprom_fwversion_ui16(void);
static eeprom_vars_t &eeprom_startup_vars();

eeprom_init_status_t eeprom_init(void) {
    static eeprom_init_status_t status = EEPROM_INIT_Undefined;
    if (status != EEPROM_INIT_Undefined)
        return status; //already initialized
    uint16_t version;
    uint16_t features;
    status = EEPROM_INIT_Normal;
    osSemaphoreDef(eepromSema);
    eeprom_sema = osSemaphoreCreate(osSemaphore(eepromSema), 1);
    st25dv64k_init();

    version = variant_get_ui16(eeprom_get_var(EEVAR_VERSION));
    features = (version >= 4) ? variant_get_ui16(eeprom_get_var(EEVAR_FEATURES)) : 0;
    if ((version >= EEPROM_FIRST_VERSION_CRC) && !eeprom_check_crc32())
        status = EEPROM_INIT_Defaults;
    else if ((version != EEPROM_VERSION) || (features != EEPROM_FEATURES)) {
        if (eeprom_convert_from(version, features) == 0) {
            status = EEPROM_INIT_Defaults;
        } else {
            status = EEPROM_INIT_Upgraded;
        }
    }
    if (status == EEPROM_INIT_Defaults)
        eeprom_defaults();
    //eeprom_print_vars(); this is not possible here because it hangs - init is now done in main.cpp, not in defaultThread
    eeprom_startup_vars(); //initialize internal static variable, first call must be done now - to ensure interrupt is enabled
    return status;
}

void eeprom_defaults(void) {
    eeprom_vars_t vars = eeprom_var_defaults;
    vars.FWBUILD = project_build_number;
    vars.FWVERSION = eeprom_fwversion_ui16();
    eeprom_lock();
    // calculate crc32
    vars.CRC32 = crc32_eeprom((uint32_t *)(&vars), (EEPROM_DATASIZE - 4) / 4);
    // write data to eeprom
    st25dv64k_user_write_bytes(EEPROM_ADDRESS, (void *)&vars, EEPROM_DATASIZE);
    eeprom_unlock();
}

static eeprom_vars_t eeprom_read_vars() {
    eeprom_vars_t ret;
    eeprom_lock();
    st25dv64k_user_read_bytes(EEPROM_ADDRESS, (void *)&ret, sizeof(ret));
    eeprom_unlock();

    return ret;
}

static eeprom_vars_t &eeprom_startup_vars() {
    static eeprom_vars_t ret = eeprom_read_vars();
    return ret;
}

variant8_t eeprom_get_var(uint8_t id) {
    uint16_t addr;
    uint16_t size;
    uint16_t data_size;
    void *data_ptr;
    variant8_t var = variant8_empty();
    if (id < EEPROM_VARCOUNT) {
        eeprom_lock();
        var = variant8_init(eeprom_map[id].type, eeprom_map[id].count, 0);
        size = eeprom_var_size(id);
        data_size = variant8_data_size(&var);
        if (size == data_size) {
            addr = eeprom_var_addr(id);
            data_ptr = variant8_data_ptr(&var);
            st25dv64k_user_read_bytes(addr, data_ptr, size);
        } else {
            _dbg("error");
            //TODO:error
        }
        eeprom_unlock();
    }
    return var;
}

void eeprom_set_var(uint8_t id, variant8_t var) {
    uint16_t addr;
    uint16_t size;
    uint16_t data_size;
    void *data_ptr;
    if (id < EEPROM_VARCOUNT) {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
        if (id == EEVAR_ZOFFSET && variant8_get_type(var) == VARIANT8_FLT) {
            variant8_t recent_sheet = eeprom_get_var(EEVAR_ACTIVE_SHEET);
            uint8_t index = variant_get_ui8(recent_sheet);
            uint16_t profile_address = eeprom_var_addr(EEVAR_SHEET_PROFILE0 + index);
            float z_offset = variant8_get_flt(var);
            st25dv64k_user_write_bytes(profile_address + MAX_SHEET_NAME_LENGTH, &z_offset, sizeof(float));
        }
#endif
        eeprom_lock();
        if (variant8_get_type(var) == eeprom_map[id].type) {
            size = eeprom_var_size(id);
            data_size = variant8_data_size(&var);
            if ((size == data_size) || ((variant8_get_type(var) == VARIANT8_PCHAR) && (data_size <= size))) {
                addr = eeprom_var_addr(id);
                data_ptr = variant8_data_ptr(&var);
                st25dv64k_user_write_bytes(addr, data_ptr, data_size);
                eeprom_update_crc32();
            } else {
                // TODO: error
            }
        } else {
            // TODO: error
        }
        eeprom_unlock();
    }
}

uint8_t eeprom_get_var_count(void) {
    return EEPROM_VARCOUNT;
}

const char *eeprom_get_var_name(uint8_t id) {
    if (id < EEPROM_VARCOUNT)
        return eeprom_map[id].name;
    return "???";
}

int eeprom_var_format(char *str, unsigned int size, uint8_t id, variant8_t var) {
    int n = 0;
    switch (id) {
    // ip addresses
    case EEVAR_LAN_IP4_ADDR:
    case EEVAR_LAN_IP4_MSK:
    case EEVAR_LAN_IP4_GW:
    case EEVAR_LAN_IP4_DNS1:
    case EEVAR_LAN_IP4_DNS2: {
        n = snprintf(str, size, "%u.%u.%u.%u", variant8_get_uia(var, 0), variant8_get_uia(var, 1),
            variant8_get_uia(var, 2), variant8_get_uia(var, 3));
    } break;
    default: //use default conversion
        n = variant8_snprintf(str, size, 0, &var);
        break;
    }
    return n;
}

void eeprom_clear(void) {
    uint16_t a;
    uint32_t data = 0xffffffff;
    eeprom_lock();
    for (a = 0x0000; a < 0x0800; a += 4) {
        st25dv64k_user_write_bytes(a, &data, 4);
        if ((a % 0x200) == 0)   // clear entire eeprom take ~4s
            wdt_iwdg_refresh(); // we will reset watchdog every ~1s for sure
    }
    eeprom_unlock();
}

// private functions

static uint16_t eeprom_var_size(uint8_t id) {
    if (id < EEPROM_VARCOUNT)
        return variant8_type_size(eeprom_map[id].type & ~VARIANT8_PTR) * eeprom_map[id].count;
    return 0;
}

static uint16_t eeprom_var_addr(uint8_t id) {
    uint16_t addr = EEPROM_ADDRESS;
    while (id)
        addr += eeprom_var_size(--id);
    return addr;
}

/*static void eeprom_print_vars(void) {
    uint8_t id;
    char text[128];
    variant8_t var8;
    variant8_t *pvar = &var8;
    for (id = 0; id < EEPROM_VARCOUNT; id++) {
        var8 = eeprom_get_var(id);
        eeprom_var_format(text, sizeof(text), id, var8);
        _dbg("%s=%s", eeprom_map[id].name, text);
        variant8_done(&pvar);
    }
}*/

static const constexpr uint16_t ADDR_V2_FILAMENT_TYPE = 0x0400;
static const constexpr uint16_t ADDR_V2_FILAMENT_COLOR = EEPROM_ADDRESS + 3;
static const constexpr uint16_t ADDR_V2_RUN_SELFTEST = EEPROM_ADDRESS + 19;
static const constexpr uint16_t ADDR_V2_ZOFFSET = 0x010e;
static const constexpr uint16_t ADDR_V2_PID_NOZ_P = 0x019d;
static const constexpr uint16_t ADDR_V2_PID_BED_P = 0x01af;

static void eeprom_save_upgraded(eeprom_vars_t &vars) {
    // calculate crc32
    vars.CRC32 = crc32_eeprom((uint32_t *)(&vars), (EEPROM_DATASIZE - 4) / 4);
    // write data to eeprom
    st25dv64k_user_write_bytes(EEPROM_ADDRESS, (void *)&vars, EEPROM_DATASIZE);
}

static void eeprom_import_block(uint8_t begin, uint8_t end, void *dst) {
    // start addres of imported data block
    uint16_t addr_start = eeprom_var_addr(begin);
    // end addres of imported data - end means the next first eeprom var we do NOT want to copy
    uint16_t addr_end = eeprom_var_addr(end);
    // read first block
    st25dv64k_user_read_bytes(addr_start, dst, addr_end - addr_start);
}

static void eeprom_init_FW_identifiers(eeprom_vars_t &vars) {
    // these variables are intentionally not initialised in eeprom_var_defaults
    vars.FWBUILD = project_build_number;
    vars.FWVERSION = eeprom_fwversion_ui16();
}

static void eeprom_make_patches(eeprom_vars_t &vars) {
    // patch active sheet profile's live-z value
    // copying the ZOFFSET var directly is safe, it has been in the eeprom at least from v2
    vars.SHEET_PROFILE0.z_offset = vars.ZOFFSET;
}

// conversion function for old version 2 format (marlin eeprom)
static int eeprom_convert_from_v2(void) {
    eeprom_vars_t vars = eeprom_var_defaults;
    eeprom_init_FW_identifiers(vars);
    // read FILAMENT_TYPE (uint8_t)
    st25dv64k_user_read_bytes(ADDR_V2_FILAMENT_TYPE, &(vars.FILAMENT_TYPE), sizeof(uint8_t));
    // initialize to zero, maybe not necessary
    if (vars.FILAMENT_TYPE == 0xff)
        vars.FILAMENT_TYPE = 0;
    // read FILAMENT_COLOR (uint32_t)
    st25dv64k_user_read_bytes(ADDR_V2_FILAMENT_COLOR, &(vars.FILAMENT_COLOR), sizeof(uint32_t));
    // read RUN_SELFTEST & RUN_XYZCALIB & RUN_FIRSTLAY & FSENSOR_ENABLED (4x uint8_t)
    st25dv64k_user_read_bytes(ADDR_V2_RUN_SELFTEST, &(vars.RUN_SELFTEST), 4 * sizeof(uint8_t));
    // read ZOFFSET (float)
    st25dv64k_user_read_bytes(ADDR_V2_ZOFFSET, &(vars.ZOFFSET), sizeof(float));
    // check ZOFFSET valid range, cancel conversion if not of valid range (defaults will be loaded)
    if ((vars.ZOFFSET < -2) || (vars.ZOFFSET > 0))
        return 0;

    eeprom_make_patches(vars);

    eeprom_save_upgraded(vars);
    return 1;
}

// conversion function for old version 4 (v 4.0.5)
static int eeprom_convert_from_v4(void) {
    eeprom_vars_t vars = eeprom_var_defaults;
    eeprom_init_FW_identifiers(vars);

    // start addres of imported data first block (FILAMENT_TYPE..EEVAR_ZOFFSET)
    eeprom_import_block(EEVAR_FILAMENT_TYPE, EEVAR_PID_NOZ_P, &(vars.FILAMENT_TYPE));

    // start addres of imported data second block (EEVAR_LAN_FLAG..EEVAR_LAN_IP4_DNS2)
    eeprom_import_block(EEVAR_LAN_FLAG, EEVAR_LAN_HOSTNAME, &(vars.LAN_FLAG));

    // TODO: keep LAN host name (?)

    eeprom_make_patches(vars);

    eeprom_save_upgraded(vars);
    return 1;
}

// conversion function for old version 6 (v 4.1.0)
static int eeprom_convert_from_v6(void) {
    eeprom_vars_t vars = eeprom_var_defaults;
    eeprom_init_FW_identifiers(vars);

    // start addres of imported data block (FILAMENT_TYPE..EEVAR_SOUND_MODE)
    eeprom_import_block(EEVAR_FILAMENT_TYPE, EEVAR_SOUND_VOLUME, &(vars.FILAMENT_TYPE));

    eeprom_make_patches(vars);

    eeprom_save_upgraded(vars);
    return 1;
}

// conversion function for old version 8 (v 4.2.x)
static int eeprom_convert_from_v7(void) {
    eeprom_vars_t vars = eeprom_var_defaults;
    eeprom_init_FW_identifiers(vars);

    // start addres of imported data block (FILAMENT_TYPE..EEVAR_LANGUAGE)
    eeprom_import_block(EEVAR_FILAMENT_TYPE, EEVAR_FILE_SORT, &(vars.FILAMENT_TYPE));

    eeprom_make_patches(vars);

    eeprom_save_upgraded(vars);
    return 1;
}

// conversion function for old version 8 (v 4.3.RC)
static int eeprom_convert_from_v8(void) {
    eeprom_vars_t vars = eeprom_var_defaults;
    eeprom_init_FW_identifiers(vars);

    // start addres of imported data block (FILAMENT_TYPE..EEVAR_SOUND_MODE)
    eeprom_import_block(EEVAR_FILAMENT_TYPE, EEVAR_MENU_TIMEOUT, &(vars.FILAMENT_TYPE));

    eeprom_make_patches(vars);

    eeprom_save_upgraded(vars);
    return 1;
}

// conversion function for old version 9 (v 4.3.2)
static int eeprom_convert_from_v9(void) {
    eeprom_vars_t vars = eeprom_var_defaults;
    eeprom_init_FW_identifiers(vars);

    // start addres of imported data block (FILAMENT_TYPE..EEVAR_DEVHASH_IN_QR)
    eeprom_import_block(EEVAR_FILAMENT_TYPE, EEVAR_FOOTER_SETTING, &(vars.FILAMENT_TYPE));

    eeprom_make_patches(vars);

    eeprom_save_upgraded(vars);
    return 1;
}

// conversion function for new version format (features, firmware version/build)
static int eeprom_convert_from(uint16_t version, uint16_t features) {
    if (version == 2)
        return eeprom_convert_from_v2();
    if (version == 4)
        return eeprom_convert_from_v4();
    if (version == 6)
        return eeprom_convert_from_v6();
    if (version == 7)
        return eeprom_convert_from_v7();
    if (version == 8)
        return eeprom_convert_from_v8();
    if (version == 9)
        return eeprom_convert_from_v9();
    return 0;
}

// version independent crc32 check
static int eeprom_check_crc32(void) {
    uint16_t datasize;
    uint32_t crc;
    datasize = variant_get_ui16(eeprom_get_var(EEVAR_DATASIZE));
    if (datasize > EEPROM_MAX_DATASIZE)
        return 0;
    st25dv64k_user_read_bytes(EEPROM_ADDRESS + datasize - 4, &crc, 4);
#if 1 //simple method
    uint8_t data[EEPROM_MAX_DATASIZE];
    uint32_t crc2;
    st25dv64k_user_read_bytes(EEPROM_ADDRESS, data, datasize);
    crc2 = crc32_eeprom((uint32_t *)data, (datasize - 4) / 4);
    return (crc == crc2) ? 1 : 0;
#else //
#endif
}

static void eeprom_update_crc32() {
#ifdef EEPROM_MEASURE_CRC_TIME
    uint32_t time = _microseconds();
#endif
#if 1 //simple method
    eeprom_vars_t vars;
    // read eeprom data
    st25dv64k_user_read_bytes(EEPROM_ADDRESS, (void *)&vars, EEPROM_DATASIZE);
    // calculate crc32
    vars.CRC32 = crc32_eeprom((uint32_t *)(&vars), (EEPROM_DATASIZE - 4) / 4);
    // write crc to eeprom
    st25dv64k_user_write_bytes(EEPROM_ADDRESS + EEPROM_DATASIZE - 4, &(vars.CRC32), 4);
#else //
#endif
#ifdef EEPROM_MEASURE_CRC_TIME
    time = _microseconds() - time;
    _dbg("crc update %u us", time);
#endif
}

static uint16_t eeprom_fwversion_ui16(void) {
    int maj = 0;
    int min = 0;
    int sub = 0;
    if (sscanf(project_version, "%d.%d.%d", &maj, &min, &sub) == 3)
        return sub + 10 * (min + 10 * maj);
    return 0;
}

int8_t eeprom_test_PUT(const unsigned int bytes) {
    unsigned int i;
    char line[16] = "abcdefghijklmno";
    char line2[16];
    uint8_t size = sizeof(line);
    unsigned int count = bytes / 16;

    for (i = 0; i < count; i++) {
        st25dv64k_user_write_bytes(EEPROM_ADDRESS + i * size, &line, size);
        if ((i % 16) == 0)
            wdt_iwdg_refresh();
    }

    int8_t res_flag = 1;

    for (i = 0; i < count; i++) {
        st25dv64k_user_read_bytes(EEPROM_ADDRESS + i * size, &line2, size);
        if (strcmp(line2, line))
            res_flag = 0;
        if ((i % 16) == 0)
            wdt_iwdg_refresh();
    }
    return res_flag;
}

///////////////////////////////////////////////////////////////////////////////
/// Sheets profile methods
uint32_t sheet_next_calibrated() {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    uint8_t index = variant_get_ui8(eeprom_get_var(EEVAR_ACTIVE_SHEET));

    for (int8_t i = 1; i < MAX_SHEETS; ++i) {
        if (sheet_select((index + i) % MAX_SHEETS))
            return (index + i) % MAX_SHEETS;
    }
#endif
    return 0;
}

bool sheet_is_calibrated(uint32_t index) {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    uint16_t profile_address = eeprom_var_addr(EEVAR_SHEET_PROFILE0 + index);
    float z_offset = FLT_MAX;
    st25dv64k_user_read_bytes(profile_address + MAX_SHEET_NAME_LENGTH,
        &z_offset, sizeof(float));
    return !nearlyEqual(z_offset, FLT_MAX, 0.001f);
#else
    return index == 0;
#endif
}

bool sheet_select(uint32_t index) {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    if (index >= MAX_SHEETS || !sheet_is_calibrated(index))
        return false;
    uint16_t active_sheet_address = eeprom_var_addr(EEVAR_ACTIVE_SHEET);
    uint16_t profile_address = eeprom_var_addr(EEVAR_SHEET_PROFILE0 + index);
    uint16_t z_offset_address = eeprom_var_addr(EEVAR_ZOFFSET);
    float z_offset = FLT_MAX;
    st25dv64k_user_read_bytes(profile_address + MAX_SHEET_NAME_LENGTH,
        &z_offset, sizeof(float));
    st25dv64k_user_write(active_sheet_address, static_cast<uint8_t>(index));
    st25dv64k_user_write_bytes(z_offset_address, &z_offset, sizeof(float));
    eeprom_update_crc32();
    return true;
#else
    return index == 0;
#endif
}

bool sheet_calibrate(uint32_t index) {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    if (index >= MAX_SHEETS)
        return false;
    uint16_t active_sheet_address = eeprom_var_addr(EEVAR_ACTIVE_SHEET);

    st25dv64k_user_write(active_sheet_address, static_cast<uint8_t>(index));
    eeprom_update_crc32();
    return true;
#else
    return index == 0;
#endif
}

bool sheet_reset(uint32_t index) {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    if (index >= MAX_SHEETS)
        return false;
    uint8_t active = variant_get_ui8(eeprom_get_var(EEVAR_ACTIVE_SHEET));
    uint16_t profile_address = eeprom_var_addr(EEVAR_SHEET_PROFILE0 + index);
    float z_offset = FLT_MAX;

    st25dv64k_user_write_bytes(profile_address + MAX_SHEET_NAME_LENGTH,
        &z_offset, sizeof(float));
    eeprom_update_crc32();
    if (active == index)
        sheet_next_calibrated();
    return true;
#else
    return false;
#endif
}

uint32_t sheet_number_of_calibrated() {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    uint32_t count = 1;
    for (int8_t i = 1; i < MAX_SHEETS; ++i) {
        if (sheet_is_calibrated(i))
            ++count;
    }
    return count;
#else
    return 1;
#endif
}

uint32_t sheet_active_name(char *buffer, uint32_t length) {
    if (!buffer || !length)
        return 0;
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    uint8_t index = variant_get_ui8(eeprom_get_var(EEVAR_ACTIVE_SHEET));
    return sheet_name(index, buffer, length);
#else
    memcpy(buffer, "DEFAULT", MAX_SHEET_NAME_LENGTH - 1);
    return MAX_SHEET_NAME_LENGTH - 1;
#endif
}

uint32_t sheet_name(uint32_t index, char *buffer, uint32_t length) {
    if (index >= MAX_SHEETS || !buffer || !length)
        return 0;
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    uint16_t profile_address = eeprom_var_addr(EEVAR_SHEET_PROFILE0 + index);
    uint32_t l = length < MAX_SHEET_NAME_LENGTH - 1
        ? length
        : MAX_SHEET_NAME_LENGTH - 1;
    st25dv64k_user_read_bytes(profile_address,
        buffer, l);
    while (l > 0 && !buffer[l - 1])
        --l;
    return l;
#else
    static const char def[] = "DEFAULT";
    memcpy(buffer, def, MAX_SHEET_NAME_LENGTH - 1);
    return MAX_SHEET_NAME_LENGTH - 1;
#endif
}

uint32_t sheet_rename(uint32_t index, char const *name, uint32_t length) {
#if (EEPROM_FEATURES & EEPROM_FEATURE_SHEETS)
    if (index >= MAX_SHEETS || !name || !length)
        return false;
    char eeprom_name[MAX_SHEET_NAME_LENGTH];
    uint16_t profile_address = eeprom_var_addr(EEVAR_SHEET_PROFILE0 + index);
    uint32_t l = length < MAX_SHEET_NAME_LENGTH - 1
        ? length
        : MAX_SHEET_NAME_LENGTH - 1;
    memset(eeprom_name, 0, MAX_SHEET_NAME_LENGTH);
    memcpy(eeprom_name, name, l);
    st25dv64k_user_write_bytes(profile_address, eeprom_name, MAX_SHEET_NAME_LENGTH);
    eeprom_update_crc32();
    return l;
#else
    return 0;
#endif
}

/*****************************************************************************/
//AXIS_Z_MAX_POS_MM
extern "C" float get_z_max_pos_mm() {
    float ret = eeprom_startup_vars().AXIS_Z_MAX_POS_MM;
    if ((ret > Z_MAX_LEN_LIMIT) || (ret < Z_MIN_LEN_LIMIT))
        ret = DEFAULT_Z_MAX_POS;
    return ret;
}

extern "C" uint16_t get_z_max_pos_mm_rounded() {
    return static_cast<uint16_t>(std::lround(get_z_max_pos_mm()));
}

extern "C" void set_z_max_pos_mm(float max_pos) {
    if ((max_pos >= Z_MIN_LEN_LIMIT) && (max_pos <= Z_MAX_LEN_LIMIT)) {
        eeprom_set_var(AXIS_Z_MAX_POS_MM, variant8_flt(max_pos));
    }
}

/*****************************************************************************/
//AXIS_STEPS_PER_UNIT
extern "C" float get_steps_per_unit_x() {
    return std::abs(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_X);
}
extern "C" float get_steps_per_unit_y() {
    return std::abs(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_Y);
}
extern "C" float get_steps_per_unit_z() {
    return std::abs(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_Z);
}
extern "C" float get_steps_per_unit_e() {
    return std::abs(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_E0);
}

extern "C" bool has_inverted_x() {
    return std::signbit(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_X);
}

extern "C" bool has_inverted_y() {
    return std::signbit(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_Y);
}

extern "C" bool has_inverted_z() {
    return std::signbit(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_Z);
}

extern "C" bool has_inverted_e() {
    return std::signbit(eeprom_startup_vars().AXIS_STEPS_PER_UNIT_E0);
}

extern "C" bool has_wrong_x() {
    return has_inverted_x() != DEFAULT_INVERT_X_DIR;
}

extern "C" bool has_wrong_y() {
    return has_inverted_y() != DEFAULT_INVERT_Y_DIR;
}

extern "C" bool has_wrong_z() {
    return has_inverted_z() != DEFAULT_INVERT_Z_DIR;
}

extern "C" bool has_wrong_e() {
    return has_inverted_e() != DEFAULT_INVERT_E0_DIR;
}

extern "C" uint16_t get_steps_per_unit_x_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_x()));
}
extern "C" uint16_t get_steps_per_unit_y_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_y()));
}
extern "C" uint16_t get_steps_per_unit_z_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_z()));
}
extern "C" uint16_t get_steps_per_unit_e_rounded() {
    return static_cast<uint16_t>(std::lround(get_steps_per_unit_e()));
}

//by write functions, cannot read startup variables, must read current value from eeprom
template <int ENUM>
bool is_current_axis_value_inverted() {
    return std::signbit(variant8_get_flt(eeprom_get_var(ENUM)));
}

template <int ENUM>
void set_steps_per_unit(float steps) {
    if (steps > 0) {
        bool negative_direction = is_current_axis_value_inverted<ENUM>();
        eeprom_set_var(ENUM, variant8_flt(negative_direction ? -steps : steps));
    }
}

extern "C" void set_steps_per_unit_x(float steps) {
    set_steps_per_unit<AXIS_STEPS_PER_UNIT_X>(steps);
}
extern "C" void set_steps_per_unit_y(float steps) {
    set_steps_per_unit<AXIS_STEPS_PER_UNIT_Y>(steps);
}
extern "C" void set_steps_per_unit_z(float steps) {
    set_steps_per_unit<AXIS_STEPS_PER_UNIT_Z>(steps);
}
extern "C" void set_steps_per_unit_e(float steps) {
    set_steps_per_unit<AXIS_STEPS_PER_UNIT_E0>(steps);
}

//by write functions, cannot read startup variables, must read current value from eeprom
template <int ENUM>
float get_current_steps_per_unit() {
    return std::abs(variant8_get_flt(eeprom_get_var(ENUM)));
}

template <int ENUM>
void set_axis_positive_direction() {
    float steps = get_current_steps_per_unit<ENUM>();
    eeprom_set_var(ENUM, variant8_flt(steps));
}

extern "C" void set_positive_direction_x() {
    set_axis_positive_direction<AXIS_STEPS_PER_UNIT_X>();
}
extern "C" void set_positive_direction_y() {
    set_axis_positive_direction<AXIS_STEPS_PER_UNIT_Y>();
}
extern "C" void set_positive_direction_z() {
    set_axis_positive_direction<AXIS_STEPS_PER_UNIT_Z>();
}
extern "C" void set_positive_direction_e() {
    set_axis_positive_direction<AXIS_STEPS_PER_UNIT_E0>();
}

template <int ENUM>
void set_axis_negative_direction() {
    float steps = get_current_steps_per_unit<ENUM>();
    eeprom_set_var(ENUM, variant8_flt(-steps));
}

extern "C" void set_negative_direction_x() {
    set_axis_negative_direction<AXIS_STEPS_PER_UNIT_X>();
}
extern "C" void set_negative_direction_y() {
    set_axis_negative_direction<AXIS_STEPS_PER_UNIT_Y>();
}
extern "C" void set_negative_direction_z() {
    set_axis_negative_direction<AXIS_STEPS_PER_UNIT_Z>();
}
extern "C" void set_negative_direction_e() {
    set_axis_negative_direction<AXIS_STEPS_PER_UNIT_E0>();
}

extern "C" void set_wrong_direction_x() {
    (!DEFAULT_INVERT_X_DIR) ? set_negative_direction_x() : set_positive_direction_x();
}
extern "C" void set_wrong_direction_y() {
    (!DEFAULT_INVERT_Y_DIR) ? set_negative_direction_y() : set_positive_direction_y();
}
extern "C" void set_wrong_direction_z() {
    (!DEFAULT_INVERT_Z_DIR) ? set_negative_direction_z() : set_positive_direction_z();
}
extern "C" void set_wrong_direction_e() {
    (!DEFAULT_INVERT_E0_DIR) ? set_negative_direction_e() : set_positive_direction_e();
}
extern "C" void set_PRUSA_direction_x() {
    DEFAULT_INVERT_X_DIR ? set_negative_direction_x() : set_positive_direction_x();
}
extern "C" void set_PRUSA_direction_y() {
    DEFAULT_INVERT_Y_DIR ? set_negative_direction_y() : set_positive_direction_y();
}
extern "C" void set_PRUSA_direction_z() {
    DEFAULT_INVERT_Z_DIR ? set_negative_direction_z() : set_positive_direction_z();
}
extern "C" void set_PRUSA_direction_e() {
    DEFAULT_INVERT_E0_DIR ? set_negative_direction_e() : set_positive_direction_e();
}

/*****************************************************************************/
//AXIS_MICROSTEPS
bool is_microstep_value_valid(uint16_t microsteps) {
    std::bitset<16> bs(microsteps);
    return bs.count() == 1; // 1,2,4,8...
}

//return default value if eeprom value is invalid
extern "C" uint16_t get_microsteps_x() {
    uint16_t ret = eeprom_startup_vars().AXIS_MICROSTEPS_X;
    return is_microstep_value_valid(ret) ? ret : X_MICROSTEPS;
}
extern "C" uint16_t get_microsteps_y() {
    uint16_t ret = eeprom_startup_vars().AXIS_MICROSTEPS_Y;
    return is_microstep_value_valid(ret) ? ret : Y_MICROSTEPS;
}
extern "C" uint16_t get_microsteps_z() {
    uint16_t ret = eeprom_startup_vars().AXIS_MICROSTEPS_Z;
    return is_microstep_value_valid(ret) ? ret : Z_MICROSTEPS;
}
extern "C" uint16_t get_microsteps_e() {
    uint16_t ret = eeprom_startup_vars().AXIS_MICROSTEPS_E0;
    return is_microstep_value_valid(ret) ? ret : E0_MICROSTEPS;
}

template <int ENUM>
void set_microsteps(uint16_t microsteps) {
    if (is_microstep_value_valid(microsteps)) {
        eeprom_set_var(ENUM, variant8_ui16(microsteps));
    }
}

extern "C" void set_microsteps_x(uint16_t microsteps) {
    set_microsteps<AXIS_MICROSTEPS_X>(microsteps);
}
extern "C" void set_microsteps_y(uint16_t microsteps) {
    set_microsteps<AXIS_MICROSTEPS_Y>(microsteps);
}
extern "C" void set_microsteps_z(uint16_t microsteps) {
    set_microsteps<AXIS_MICROSTEPS_Z>(microsteps);
}
extern "C" void set_microsteps_e(uint16_t microsteps) {
    set_microsteps<AXIS_MICROSTEPS_E0>(microsteps);
}

/*****************************************************************************/
//AXIS_RMS_CURRENT_MA_X
//current must be > 0, return default value if it is not
extern "C" uint16_t get_rms_current_ma_x() {
    uint16_t ret = eeprom_startup_vars().AXIS_RMS_CURRENT_MA_X;
    return (ret > 0) ? ret : X_CURRENT;
}
extern "C" uint16_t get_rms_current_ma_y() {
    uint16_t ret = eeprom_startup_vars().AXIS_RMS_CURRENT_MA_Y;
    return (ret > 0) ? ret : Y_CURRENT;
}
extern "C" uint16_t get_rms_current_ma_z() {
    uint16_t ret = eeprom_startup_vars().AXIS_RMS_CURRENT_MA_Z;
    return (ret > 0) ? ret : Z_CURRENT;
}
extern "C" uint16_t get_rms_current_ma_e() {
    uint16_t ret = eeprom_startup_vars().AXIS_RMS_CURRENT_MA_E0;
    return (ret > 0) ? ret : E0_CURRENT;
}

template <int ENUM>
void set_rms_current_ma(uint16_t current) {
    if (current > 0) {
        eeprom_set_var(ENUM, variant8_ui16(current));
    }
}

extern "C" void set_rms_current_ma_x(uint16_t current) {
    set_rms_current_ma<AXIS_RMS_CURRENT_MA_X>(current);
}
extern "C" void set_rms_current_ma_y(uint16_t current) {
    set_rms_current_ma<AXIS_RMS_CURRENT_MA_Y>(current);
}
extern "C" void set_rms_current_ma_z(uint16_t current) {
    set_rms_current_ma<AXIS_RMS_CURRENT_MA_Z>(current);
}
extern "C" void set_rms_current_ma_e(uint16_t current) {
    set_rms_current_ma<AXIS_RMS_CURRENT_MA_E0>(current);
}
