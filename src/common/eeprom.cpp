// eeprom.cpp

#include <cassert>
#include <string.h>
#include <float.h>

#include "eeprom.h"
#include "eeprom_function_api.h"
#include "st25dv64k.h"
#include "log.h"
#include "timing.h"
#include "cmsis_os.h"
#include "crc32.h"
#include "version.h"
#include "wdt.h"
#include "../Marlin/src/module/temperature.h"
#include "cmath_ext.h"
#include "footer_eeprom.hpp"
#include <bitset>
#include "eeprom_current.hpp"
#include "bsod.h"
using namespace eeprom::current;

LOG_COMPONENT_DEF(EEPROM, LOG_SEVERITY_INFO);

static const constexpr uint8_t EEPROM_MAX_NAME = 16;       // maximum name length (with '\0')
static const constexpr uint16_t EEPROM_MAX_DATASIZE = 576; // maximum datasize

// flags will be used also for selective variable reset default values in some cases (shipping etc.))
static const constexpr uint16_t EEVAR_FLG_READONLY = 0x0001; // variable is read only

// this pragma pack must remain intact, the ordering of EEPROM variables is not alignment-friendly
#pragma pack(push, 1)

// eeprom map entry structure
struct eeprom_entry_t {
    const char name[EEPROM_MAX_NAME];
    uint8_t type;   // variant8 data type
    uint8_t count;  // number of elements
    uint16_t flags; // flags
};

struct eeprom_head_t {
    uint16_t VERSION;
    uint16_t FEATURES;
    uint16_t DATASIZE;
    uint16_t FWVERSION;
    uint16_t FWBUILD;
};

// eeprom vars structure (used for defaults, packed - see above pragma)
struct eeprom_vars_t {
    eeprom_head_t head;
    vars_body_t body;
    uint32_t CRC32;
};

/**
 * @brief union containing eeprom struct and entire eeprom area
 * area (data) is needed for old eeprom version update and crc verification
 * because old eeprom could be bigger then current
 */
union eeprom_data {
    uint8_t data[EEPROM_MAX_DATASIZE];
    eeprom_vars_t vars;
    struct {
        eeprom_head_t head;
        union {
            eeprom::v4::vars_body_t v4;
            eeprom::v6::vars_body_t v6;
            eeprom::v7::vars_body_t v7;
            eeprom::v9::vars_body_t v9;
            eeprom::v10::vars_body_t v10;
            eeprom::v11::vars_body_t v11;
            eeprom::current::vars_body_t current;
        };
    };
};
#pragma pack(pop)

static_assert(sizeof(eeprom_vars_t) == sizeof(eeprom_head_t) + sizeof(vars_body_t) + sizeof(uint32_t), "Wrong size of eeprom. Missing pragma pack? ");

// uncomment to check eeprom size .. will cause error
// char (*__eeprom_size_check)[sizeof( eeprom_vars_t )] = 1;

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
    { "RUN_SELFTEST",    VARIANT8_BOOL,  1, 0 }, // EEVAR_RUN_SELFTEST
    { "RUN_XYZCALIB",    VARIANT8_BOOL,  1, 0 }, // EEVAR_RUN_XYZCALIB
    { "RUN_FIRSTLAY",    VARIANT8_BOOL,  1, 0 }, // EEVAR_RUN_FIRSTLAY
    { "FSENSOR_ENABLED", VARIANT8_BOOL,   1, 0 }, // EEVAR_FSENSOR_ENABLED
    { "ZOFFSET",         VARIANT8_FLT,   1, 0 }, // EEVAR_ZOFFSET_DO_NOT_USE_DIRECTLY
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
    { "MENU_TIMEOUT",    VARIANT8_BOOL,  1, 0 }, // EEVAR_MENU_TIMEOUT
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
    { "DEVHASH_IN_QR",   VARIANT8_BOOL,  1, 0 }, // EEVAR_DEVHASH_IN_QR
    { "FOOTER_SETTING",  VARIANT8_UI32,  1, 0 }, // EEVAR_FOOTER_SETTING
    { "FOOTER_DRAW_TP"  ,VARIANT8_UI32,  1, 0 }, // EEVAR_FOOTER_DRAW_TYPE
    { "FAN_CHECK_ENA",   VARIANT8_BOOL,  1, 0 }, // EEVAR_FAN_CHECK_ENABLED
    { "FS_AUTOL_ENA",    VARIANT8_BOOL,  1, 0},  // EEVAR_FS_AUTOLOAD_ENABLED
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
    { "ACTIVE_NETDEV",   VARIANT8_UI8,   1, 0 },
    { "PL_RUN",          VARIANT8_UI8,   1, 0 },    // EEVAR_PL_RUN
    { "PL_PASSWORD",      VARIANT8_PCHAR, PL_PASSWORD_SIZE, 0 }, // EEVAR_PL_PASSWORD
    { "WIFI_FLAG",       VARIANT8_UI8,   1, 0 }, // EEVAR_WIFI_FLAG
    { "WIFI_IP4_ADDR",   VARIANT8_UI32,  1, 0 }, // EEVAR_WIFI_IP4_ADDR
    { "WIFI_IP4_MSK",    VARIANT8_UI32,  1, 0 }, // EEVAR_WIFI_IP4_MSK
    { "WIFI_IP4_GW",     VARIANT8_UI32,  1, 0 }, // EEVAR_WIFI_IP4_GW
    { "WIFI_IP4_DNS1",   VARIANT8_UI32,  1, 0 }, // EEVAR_WIFI_IP4_DNS1
    { "WIFI_IP4_DNS2",   VARIANT8_UI32,  1, 0 }, // EEVAR_WIFI_IP4_DNS2
    { "WIFI_HOSTNAME",   VARIANT8_PCHAR, LAN_HOSTNAME_MAX_LEN + 1, 0 }, // EEVAR_WIFI_HOSTNAME
    { "WIFI_AP_SSID",    VARIANT8_PCHAR, WIFI_MAX_SSID_LEN + 1, 0 }, // EEVAR_WIFI_HOSTNAME
    { "WIFI_AP_PASSWD",  VARIANT8_PCHAR, WIFI_MAX_PASSWD_LEN + 1, 0 }, // EEVAR_WIFI_HOSTNAME
    { "USB_MSC_ENABLED", VARIANT8_BOOL,  1, 0 }, // EEVAR_USB_MSC_ENABLED
    { "CONNECT_HOST",    VARIANT8_PCHAR, CONNECT_HOST_SIZE + 1, 0 }, // EEVAR_CONNECT_HOST
    { "CONNECT_TOKEN",   VARIANT8_PCHAR, CONNECT_TOKEN_SIZE + 1, 0 }, // EEVAR_CONNECT_TOKEN
    { "CONNECT_PORT",    VARIANT8_UI16,  1, 0 }, // EEVAR_CONNECT_PORT
    { "CONNECT_TLS",     VARIANT8_BOOL,  1, 0 }, // EEVAR_CONNECT_TLS
    { "CONNECT_ENABLED", VARIANT8_BOOL,  1, 0 }, // EEVAR_CONNECT_ENABLED
    { "JOB_ID",          VARIANT8_UI16,  1, 0 }, // EEVAR_JOB_ID
    { "CRASH_ENABLED",   VARIANT8_BOOL,  1, 0 }, // EEVAR_CRASH_ENABLED
    { "CRASH_SENS_X",    VARIANT8_I16,    1, 0 }, // EEVAR_CRASH_SENS_X,
    { "CRASH_SENS_Y",    VARIANT8_I16,    1, 0 }, // EEVAR_CRASH_SENS_Y,
    { "CRASH_PERIOD_X",  VARIANT8_UI16,  1, 0 }, // EEVAR_CRASH_PERIOD_X,
    { "CRASH_PERIOD_Y",  VARIANT8_UI16,  1, 0 }, // EEVAR_CRASH_PERIOD_Y,
    { "CRASH_FILTER",    VARIANT8_BOOL,  1, 0 }, // EEVAR_CRASH_FILTER,
    { "CRASH_X",         VARIANT8_UI16,  1, 0 }, // EEVAR_CRASH_COUNT_X_TOT
    { "CRASH_Y",         VARIANT8_UI16,  1, 0 }, // EEVAR_CRASH_COUNT_Y_TOT
    { "POWER_PANIC",     VARIANT8_UI16,  1, 0 }, // EEVAR_POWER_COUNT_TOT
// crc
    { "CRC32",           VARIANT8_UI32,  1, 0 }, // EEVAR_CRC32
};

static const constexpr uint32_t EEPROM_VARCOUNT = sizeof(eeprom_map) / sizeof(eeprom_entry_t);
static const constexpr uint32_t EEPROM_DATASIZE = sizeof(eeprom_vars_t);

static constexpr eeprom_head_t eeprom_head_defaults = {
    EEPROM_VERSION,  // EEVAR_VERSION
    EEPROM_FEATURES, // EEVAR_FEATURES
    EEPROM_DATASIZE, // EEVAR_DATASIZE
    0,               // EEVAR_FW_VERSION
    0,               // EEVAR_FW_BUILD
};

// eeprom variable defaults
static const eeprom_vars_t eeprom_var_defaults = {
    eeprom_head_defaults,
    body_defaults,
    0xffffffff,      // EEVAR_CRC32
};
// clang-format on

static_assert(EEPROM_DATASIZE < EEPROM_MAX_DATASIZE, "eeprom datasize is bigger then EEPROM_MAX_DATASIZE");

// semaphore handle (lock/unlock)
// zero initialized variable is fine even during initialization called from startup script
static osSemaphoreId eeprom_sema = 0;

static inline void eeprom_lock(void) {
    osSemaphoreWait(eeprom_sema, osWaitForever);
}

static inline void eeprom_unlock(void) {
    osSemaphoreRelease(eeprom_sema);
}

static eeprom_data eeprom_ram_mirror; // must be zero initialized

static void eeprom_init_ram_mirror() {
    eeprom_lock();
    st25dv64k_user_read_bytes(EEPROM_ADDRESS, (void *)&eeprom_ram_mirror, sizeof(eeprom_ram_mirror));
    eeprom_unlock();
}

/**
 * @brief function returning reference to eeprom struct in RAM
 *
 * @return eeprom_vars_t& reference to eeprom struct
 */
static constexpr eeprom_vars_t &eeprom_startup_vars() {
    return eeprom_ram_mirror.vars;
}

static constexpr bool is_version_supported(uint16_t version) {
#ifdef NO_EEPROM_UPGRADES
    return version == EEPROM_VERSION;
#else
    // supported versions are 4,6,7,9,10 .. current
    if (version > EEPROM_VERSION)
        return false;
    if (version == 4)
        return true;
    if (version == 6)
        return true;
    if (version == 7)
        return true;
    return version >= 9;
#endif
};

// forward declarations of private functions
static void eeprom_set_var(enum eevar_id id, void const *var_ptr, size_t var_size);
static void eeprom_get_var(enum eevar_id id, void *var_ptr, size_t var_size);
static void eeprom_write_vars();
static uint16_t eeprom_var_size(enum eevar_id id);
static uint16_t eeprom_var_addr(enum eevar_id id, uint16_t addr = EEPROM_ADDRESS);
static void *eeprom_var_ptr(enum eevar_id id, eeprom_vars_t &pVars);
static bool eeprom_convert_from(eeprom_data &data);

static bool eeprom_check_crc32();
static void update_crc32_both_ram_eeprom(eeprom_vars_t &eevars);
static void update_crc32_in_ram(eeprom_vars_t &eevars);

static uint16_t eeprom_fwversion_ui16(void);

eeprom_init_status_t eeprom_init(void) {
    static eeprom_init_status_t status = EEPROM_INIT_Undefined;
    if (status != EEPROM_INIT_Undefined)
        return status; // already initialized
    status = EEPROM_INIT_Normal;
    osSemaphoreDef(eepromSema);
    eeprom_sema = osSemaphoreCreate(osSemaphore(eepromSema), 1);
    st25dv64k_init();

    eeprom_init_ram_mirror();
    eeprom_vars_t &eevars = eeprom_startup_vars();

    if (!eeprom_check_crc32())
        status = EEPROM_INIT_Defaults;
    else if ((eevars.head.VERSION != EEPROM_VERSION) || (eevars.head.FEATURES != EEPROM_FEATURES)) {
        if (eeprom_convert_from(eeprom_ram_mirror)) {
            status = EEPROM_INIT_Upgraded;
        } else {
            status = EEPROM_INIT_Defaults;
        }
    }
    switch (status) {
    case EEPROM_INIT_Defaults:
        eeprom_defaults();
        break;
    case EEPROM_INIT_Upgraded:
        eeprom_write_vars();
        break;
    default:
        break;
    }

    return status;
}

static void eeprom_write_vars() {
    eeprom_vars_t &vars = eeprom_startup_vars();
    eeprom_lock();
    vars.CRC32 = crc32_calc((uint8_t *)(&vars), EEPROM_DATASIZE - 4);
    // write data to eeprom
    st25dv64k_user_write_bytes(EEPROM_ADDRESS, (void *)&vars, EEPROM_DATASIZE);
    eeprom_unlock();
}

void eeprom_defaults_RAM() {
    eeprom_vars_t &vars = eeprom_startup_vars();
    vars = eeprom_var_defaults;
    vars.head.FWBUILD = project_build_number;
    vars.head.FWVERSION = eeprom_fwversion_ui16();
}

void eeprom_defaults() {
    eeprom_defaults_RAM();
    eeprom_write_vars();
}

/**
 * @brief reads eeprom record from RAM structure into variant variable
 *
 * @param id eeprom record index
 * @return variant8_t eeprom record
 */
variant8_t eeprom_get_var(enum eevar_id id) {
    variant8_t var = variant8_empty();
    if (id < EEPROM_VARCOUNT) {
        var = variant8_init(eeprom_map[id].type, eeprom_map[id].count, 0);
        eeprom_get_var(id, variant8_data_ptr(&var), variant8_data_size(&var));
    } else {
        assert(0 /* EEProm var Id out of range */);
    }
    return var;
}

float eeprom_get_flt(enum eevar_id id) { return variant8_get_flt(eeprom_get_var(id)); }
char *eeprom_get_pch(enum eevar_id id) { return variant8_get_pch(eeprom_get_var(id)); }
uint8_t eeprom_get_uia(enum eevar_id id, uint8_t index) { return variant8_get_uia(eeprom_get_var(id), index); }
uint32_t eeprom_get_ui32(enum eevar_id id) { return variant8_get_ui32(eeprom_get_var(id)); }
int32_t eeprom_get_i32(enum eevar_id id) { return variant8_get_i32(eeprom_get_var(id)); }
uint16_t eeprom_get_ui16(enum eevar_id id) { return variant8_get_ui16(eeprom_get_var(id)); }
uint8_t eeprom_get_ui8(enum eevar_id id) { return variant8_get_ui8(eeprom_get_var(id)); }
int8_t eeprom_get_i8(enum eevar_id id) { return variant8_get_i8(eeprom_get_var(id)); }
bool eeprom_get_bool(enum eevar_id id) { return variant8_get_bool(eeprom_get_var(id)); }
/// Gets sheet from eeprom
/// \param index
/// \returns sheet from eepro, if index out of range returns Sheet{UNDEF,def_val}
Sheet eeprom_get_sheet(uint32_t index) {
    if (index > eeprom_num_sheets) {
        return Sheet { "UNDEF", eeprom_z_offset_uncalibrated };
    }
    Sheet sheet;
    eeprom_get_var(static_cast<enum eevar_id>(EEVAR_SHEET_PROFILE0 + index), &sheet, sizeof(sheet));
    return sheet;
}

/**
 * @brief reads eeprom record from RAM structure
 *
 * @param id eeprom record index
 * @param var_ptr pointer to variable to be read
 * @param var_size size of variable
 */
static void eeprom_get_var(enum eevar_id id, void *var_ptr, size_t var_size) {
    if (id < EEPROM_VARCOUNT) {
        size_t size = eeprom_var_size(id);
        if (size == var_size) {
            eeprom_vars_t &vars = eeprom_startup_vars();
            const void *var_addr = eeprom_var_ptr(id, vars);
            // need to lock even if it is not accesing eeprom
            // because RAM structure changes during write also
            eeprom_lock();
            memcpy(var_ptr, var_addr, size);
            eeprom_unlock();
        } else {
            // TODO:error
            log_error(EEPROM, "%s: invalid data size", __FUNCTION__);
        }
    } else {
        assert(0 /* EEProm var Id out of range */);
    }
}

/**
 * @brief function that writes variant8_t to eeprom, also actualizes crc and RAM structure
 *
 * @param id eeprom record index
 * @param var variant variable holding data to be written
 */
void eeprom_set_var(enum eevar_id id, variant8_t var) {
    if (id < EEPROM_VARCOUNT) {
        if (variant8_get_type(var) == eeprom_map[id].type) {
            eeprom_set_var(id, variant8_data_ptr(&var), eeprom_var_size(id));
        } else {
            // TODO: error
            log_error(EEPROM, "%s: variant type missmatch on id: %x", __FUNCTION__, id);
        }
    } else {
        assert(0 /* EEProm var Id out of range */);
    }
}

/**
 * @brief function that writes data to eeprom, also actualizes crc and RAM structure
 *
 * If the same value is in EEPROM then no writing is done.
 * @param id eeprom record index
 * @param var_ptr pointer to variable to be written
 * @param var_size size of variable
 */
static void eeprom_set_var(enum eevar_id id, void const *var_ptr, size_t var_size) {
    if (id >= EEPROM_VARCOUNT) {
        assert(0 /* EEProm var Id out of range */);
        return;
    }
    if (var_size != eeprom_var_size(id)) {
        // TODO: error
        log_error(EEPROM, "%s: invalid data size", __FUNCTION__);
        return;
    }

    eeprom_vars_t &vars = eeprom_startup_vars();
    void *var_ram_addr = eeprom_var_ptr(id, vars);
    // critical section
    eeprom_lock();
    if (memcmp(var_ram_addr, var_ptr, var_size)) {
        memcpy(var_ram_addr, var_ptr, var_size);
        st25dv64k_user_write_bytes(eeprom_var_addr(id), var_ptr, var_size);
        update_crc32_both_ram_eeprom(vars);
    }
    eeprom_unlock();
}

void eeprom_set_i8(enum eevar_id id, int8_t i8) { eeprom_set_var(id, variant8_i8(i8)); }
void eeprom_set_bool(enum eevar_id id, bool b) { eeprom_set_var(id, variant8_bool(b)); }
void eeprom_set_ui8(enum eevar_id id, uint8_t ui8) { eeprom_set_var(id, variant8_ui8(ui8)); }
void eeprom_set_i16(enum eevar_id id, int16_t i16) { eeprom_set_var(id, variant8_i16(i16)); }
void eeprom_set_ui16(enum eevar_id id, uint16_t ui16) { eeprom_set_var(id, variant8_ui16(ui16)); }
void eeprom_set_i32(enum eevar_id id, int32_t i32) { eeprom_set_var(id, variant8_i32(i32)); }
void eeprom_set_ui32(enum eevar_id id, uint32_t ui32) { eeprom_set_var(id, variant8_ui32(ui32)); }
void eeprom_set_flt(enum eevar_id id, float flt) { eeprom_set_var(id, variant8_flt(flt)); }
void eeprom_set_pchar(enum eevar_id id, char *pch, uint16_t count, int init) {
    eeprom_set_var(id, variant8_pchar(pch, count, init));
}
/// Saves sheet to eeprom
/// \param index where to store the sheet
/// \param sheet
/// \retval true if successful
/// \retval false if index out of bound
bool eeprom_set_sheet(uint32_t index, Sheet sheet) {
    if (index > eeprom_num_sheets) {
        return false;
    }
    eeprom_set_var(static_cast<enum eevar_id>(EEVAR_SHEET_PROFILE0 + index), &sheet, sizeof(Sheet));
    return true;
}

uint8_t eeprom_get_var_count(void) {
    return EEPROM_VARCOUNT;
}

const char *eeprom_get_var_name(enum eevar_id id) {
    if (id < EEPROM_VARCOUNT)
        return eeprom_map[id].name;
    log_error(EEPROM, "%s: could not evaluate id: %x", __PRETTY_FUNCTION__, id);
    return "???";
}

bool eeprom_find_var_by_name(const char *name, enum eevar_id *var_id_out) {
    for (int i = 0; i < (int)EEPROM_VARCOUNT; i++) {
        if (strcmp(eeprom_map[i].name, name) == 0) {
            *var_id_out = (enum eevar_id)i;
            return true;
        }
    }
    return false;
}

int eeprom_var_format(char *str, unsigned int size, enum eevar_id id, variant8_t var) {
    int n = 0;
    switch (id) {
    // ip addresses
    case EEVAR_LAN_IP4_ADDR:
    case EEVAR_LAN_IP4_MSK:
    case EEVAR_LAN_IP4_GW:
    case EEVAR_LAN_IP4_DNS1:
    case EEVAR_LAN_IP4_DNS2:
    case EEVAR_WIFI_IP4_ADDR:
    case EEVAR_WIFI_IP4_MSK:
    case EEVAR_WIFI_IP4_GW:
    case EEVAR_WIFI_IP4_DNS1:
    case EEVAR_WIFI_IP4_DNS2: {
        n = snprintf(str, size, "%u.%u.%u.%u", variant8_get_uia(var, 0), variant8_get_uia(var, 1),
            variant8_get_uia(var, 2), variant8_get_uia(var, 3));
        break;
    }
    default: // use default conversion
        n = variant8_snprintf(str, size, 0, &var);
        break;
    }
    return n;
}

variant8_t eeprom_var_parse(enum eevar_id id, char *str) {
    switch (id) {
    // ip addresses
    case EEVAR_LAN_IP4_ADDR:
    case EEVAR_LAN_IP4_MSK:
    case EEVAR_LAN_IP4_GW:
    case EEVAR_LAN_IP4_DNS1:
    case EEVAR_LAN_IP4_DNS2:
    case EEVAR_WIFI_IP4_ADDR:
    case EEVAR_WIFI_IP4_MSK:
    case EEVAR_WIFI_IP4_GW:
    case EEVAR_WIFI_IP4_DNS1:
    case EEVAR_WIFI_IP4_DNS2: {
        uint8_t bytes[4];
        int read = sscanf(str, "%hhu.%hhu.%hhu.%hhu", &bytes[0], &bytes[1],
            &bytes[2], &bytes[3]);
        if (read == 4)
            return variant8_ui32(*((uint32_t *)bytes));
        else
            return variant8_error(VARIANT8_ERR_INVFMT, 0, 0);
    }
    default: // use default conversion
        return variant8_from_str(eeprom_map[id].type, str);
    }
    return variant8_error(VARIANT8_ERR_UNSCON, 0, 0);
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

static uint16_t eeprom_var_size(enum eevar_id id) {
    if (id < EEPROM_VARCOUNT)
        return variant8_type_size(eeprom_map[id].type & ~VARIANT8_PTR) * eeprom_map[id].count;
    log_error(EEPROM, "%s: could not evaluate id: %x", __PRETTY_FUNCTION__, id);
    return 0;
}

/**
 * @brief calculates address offset of given variable inside eeprom struct
 *
 * @param id variable
 * @param addr address offset in eeprom, default value EEPROM_ADDRESS
 * if struct is not in eeprom use 0
 * @return uint16_t address offset of given variable
 */
static uint16_t eeprom_var_addr(enum eevar_id id, uint16_t addr) {
    uint8_t id_idx = id;
    while (id_idx > 0)
        addr += eeprom_var_size(static_cast<enum eevar_id>(--id_idx));
    return addr;
}

/**
 * @brief return pointer to variable in given eeprom_vars_t structure
 *
 * @param id variable id
 * @param vars structure af all variables
 * @return void* pointer to wanted variable
 */
static void *eeprom_var_ptr(enum eevar_id id, eeprom_vars_t &vars) {
    uint8_t *addr = reinterpret_cast<uint8_t *>(&vars);
    uint8_t id_idx = id;
    while (id_idx > 0)
        addr += eeprom_var_size(static_cast<enum eevar_id>(--id_idx));
    return addr;
}

/**
 * @brief conversion function for new version format (features, firmware version/build)
 * does not change crc, it is changed automatically by write function
 *
 * @param eevars eeprom struct in RAM
 * @return true updated (changed)
 * @return false not changed, need reset to defaults
 */
static bool eeprom_convert_from(eeprom_data &data) {
    uint16_t version = data.head.VERSION;
    if (version == 4) {
        data.v6 = eeprom::v6::convert(data.v4);
        version = 6;
    }

    if (version == 6) {
        data.v7 = eeprom::v7::convert(data.v6);
        version = 7;
    }

    if (version == 7) {
        data.v9 = eeprom::v9::convert(data.v7);
        version = 9;
    }

    if (version == 9) {
        data.v10 = eeprom::v10::convert(data.v9);
        version = 10;
    }

    if (version == 10) {
        data.v11 = eeprom::v11::convert(data.v10);
        version = 11;
    }

    if (version == 11) {
        data.current = eeprom::current::convert(data.v11);
        version = 12;
    }

    // after body was updated we can update head
    // don't do it before body, because it will rewrite the values in head to default values
    data.head = eeprom_head_defaults;
    data.head.FWBUILD = project_build_number;
    data.head.FWVERSION = eeprom_fwversion_ui16();

    // if update was successful, version will be current
    return version == EEPROM_VERSION;
}

// version independent crc32 check
static bool eeprom_check_crc32() {
    if (eeprom_ram_mirror.vars.head.DATASIZE > EEPROM_MAX_DATASIZE)
        return false;
    if (eeprom_ram_mirror.vars.head.DATASIZE == 0)
        return false;
    if (!is_version_supported(eeprom_ram_mirror.vars.head.VERSION))
        return false;

    uint32_t crc;
    if (eeprom_ram_mirror.vars.head.VERSION <= EEPROM_LAST_VERSION_WITH_OLD_CRC) {
        crc = crc32_eeprom((uint32_t *)(&eeprom_ram_mirror), (eeprom_ram_mirror.vars.head.DATASIZE - 4) / 4);
    } else {
        crc = crc32_calc((uint8_t *)(&eeprom_ram_mirror), eeprom_ram_mirror.vars.head.DATASIZE - 4);
    }
    uint32_t crc_from_eeprom;
    memcpy(&crc_from_eeprom, eeprom_ram_mirror.data + eeprom_ram_mirror.vars.head.DATASIZE - 4, sizeof(crc_from_eeprom));
    return crc_from_eeprom == crc;
}

static void update_crc32_both_ram_eeprom(eeprom_vars_t &eevars) {
    uint32_t time = ticks_us();

    update_crc32_in_ram(eevars);
    // write crc to eeprom
    st25dv64k_user_write_bytes(EEPROM_ADDRESS + EEPROM_DATASIZE - 4, &(eevars.CRC32), 4);

    log_info(EEPROM, "CRC update took %u us", ticks_diff(ticks_us(), time));
}

static void update_crc32_in_ram(eeprom_vars_t &eevars) {
    // calculate crc32
    if (eevars.head.VERSION <= EEPROM_LAST_VERSION_WITH_OLD_CRC) {
        eevars.CRC32 = crc32_eeprom((uint32_t *)(&eevars), (EEPROM_DATASIZE - 4) / 4);
    } else {
        eevars.CRC32 = crc32_calc((uint8_t *)(&eevars), EEPROM_DATASIZE - 4);
    }
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

// AXIS_Z_MAX_POS_MM
extern "C" float get_z_max_pos_mm() {
    float ret = 0.F;
#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    ret = eeprom_startup_vars().body.AXIS_Z_MAX_POS_MM;
    if ((ret > Z_MAX_LEN_LIMIT) || (ret < Z_MIN_LEN_LIMIT))
        ret = DEFAULT_Z_MAX_POS;
    log_debug(EEPROM, "%s returned %f", __PRETTY_FUNCTION__, double(ret));
#else
    log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
#endif
    return ret;
}

extern "C" uint16_t get_z_max_pos_mm_rounded() {
    return static_cast<uint16_t>(std::lround(get_z_max_pos_mm()));
}

extern "C" void set_z_max_pos_mm(float max_pos) {
#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    if ((max_pos >= Z_MIN_LEN_LIMIT) && (max_pos <= Z_MAX_LEN_LIMIT)) {
        eeprom_set_var(AXIS_Z_MAX_POS_MM, variant8_flt(max_pos));
    }
    log_debug(EEPROM, "%s set %f", __PRETTY_FUNCTION__, double(max_pos));
#else
    log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
#endif
}

/*****************************************************************************/
// AXIS_STEPS_PER_UNIT
extern "C" float get_steps_per_unit_x() {
    return std::abs(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_X);
}
extern "C" float get_steps_per_unit_y() {
    return std::abs(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_Y);
}
extern "C" float get_steps_per_unit_z() {
    return std::abs(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_Z);
}
extern "C" float get_steps_per_unit_e() {
    return std::abs(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_E0);
}

extern "C" bool has_inverted_x() {
    return std::signbit(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_X);
}

extern "C" bool has_inverted_y() {
    return std::signbit(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_Y);
}

extern "C" bool has_inverted_z() {
    return std::signbit(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_Z);
}

extern "C" bool has_inverted_e() {
    return std::signbit(eeprom_startup_vars().body.AXIS_STEPS_PER_UNIT_E0);
}

#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
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
#else
extern "C" bool has_wrong_x() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
extern "C" bool has_wrong_y() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
extern "C" bool has_wrong_z() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
extern "C" bool has_wrong_e() {
    log_info(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__);
    return false;
}
#endif

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

// by write functions, cannot read startup variables, must read current value from eeprom
template <enum eevar_id ENUM>
bool is_current_axis_value_inverted() {
    return std::signbit(eeprom_get_flt(ENUM));
}

template <enum eevar_id ENUM>
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

// by write functions, cannot read startup variables, must read current value from eeprom
template <enum eevar_id ENUM>
float get_current_steps_per_unit() {
    return std::abs(eeprom_get_flt(ENUM));
}

template <enum eevar_id ENUM>
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

template <enum eevar_id ENUM>
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

#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
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
#else
extern "C" void set_wrong_direction_x() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_wrong_direction_y() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_wrong_direction_z() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_wrong_direction_e() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_x() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_y() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_z() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
extern "C" void set_PRUSA_direction_e() { log_error(EEPROM, "called %s while USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES is disabled", __PRETTY_FUNCTION__); }
#endif

/*****************************************************************************/
// AXIS_MICROSTEPS
bool is_microstep_value_valid(uint16_t microsteps) {
    std::bitset<16> bs(microsteps);
    return bs.count() == 1; // 1,2,4,8...
}

// return default value if eeprom value is invalid
extern "C" uint16_t get_microsteps_x() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_MICROSTEPS_X;
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = X_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_y() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_MICROSTEPS_Y;
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Y_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_z() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_MICROSTEPS_Z;
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Z_MICROSTEPS;
    }
    return ret;
}
extern "C" uint16_t get_microsteps_e() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_MICROSTEPS_E0;
    if (!is_microstep_value_valid(ret)) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = E0_MICROSTEPS;
    }
    return ret;
}

template <enum eevar_id ENUM>
void set_microsteps(uint16_t microsteps) {
    if (is_microstep_value_valid(microsteps)) {
        eeprom_set_var(ENUM, variant8_ui16(microsteps));
        log_debug(EEPROM, "%s: microsteps %d ", __PRETTY_FUNCTION__, microsteps);
    } else {
        log_error(EEPROM, "%s: microsteps %d not set", __PRETTY_FUNCTION__, microsteps);
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
// AXIS_RMS_CURRENT_MA_X
// current must be > 0, return default value if it is not
extern "C" uint16_t get_rms_current_ma_x() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_RMS_CURRENT_MA_X;
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = X_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_y() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_RMS_CURRENT_MA_Y;
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Y_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_z() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_RMS_CURRENT_MA_Z;
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = Z_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}
extern "C" uint16_t get_rms_current_ma_e() {
    uint16_t ret = eeprom_startup_vars().body.AXIS_RMS_CURRENT_MA_E0;
    if (ret == 0) {
        log_error(EEPROM, "%s: invalid value %d", __PRETTY_FUNCTION__, ret);
        ret = E0_CURRENT;
    }
    log_debug(EEPROM, "%s: returned %d ", __PRETTY_FUNCTION__, ret);
    return ret;
}

template <enum eevar_id ENUM>
void set_rms_current_ma(uint16_t current) {
    if (current > 0) {
        eeprom_set_var(ENUM, variant8_ui16(current));
        log_debug(EEPROM, "%s: current %d ", __PRETTY_FUNCTION__, current);
    } else {
        log_error(EEPROM, "%s: current must be greater than 0", __PRETTY_FUNCTION__);
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
