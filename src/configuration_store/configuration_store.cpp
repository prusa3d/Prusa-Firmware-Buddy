#include "configuration_store.hpp"
#include <eeprom.h>
#include <eeprom_v_current.hpp>
#include <type_traits>
#include <rtos_api.hpp>
#include <version.h>
#include <charconv>

/**
 * @brief Remnants of old eeprom required for migration
 *
 */
namespace old_eeprom {
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
    ::eeprom::current::vars_body_t body;
    uint32_t CRC32;
};

constexpr uint32_t EEPROM_DATASIZE = sizeof(eeprom_vars_t);
constexpr uint16_t EEPROM_MAX_DATASIZE = 1024; // maximum datasize
constexpr size_t EEPROM_DATA_INIT_TRIES = 3;   // maximum tries to read crc32 ok data on init

static_assert(EEPROM_DATASIZE <= EEPROM_MAX_DATASIZE, "EEPROM_MAX_DATASIZE might be outdated and not needed anymore, but EEPROM_DATASIZE shouldn't have increased anyway");

#if DEVELOPMENT_ITEMS()
    #define PRIVATE__EEPROM_OFFSET (1 << 15) // to avoid collision with public version
    #define NO_EEPROM_UPGRADES
#else
    #define PRIVATE__EEPROM_OFFSET 0
#endif

enum {
    EEPROM_VERSION = PRIVATE__EEPROM_OFFSET + 23, // uint16_t
};

/**
 * @brief union containing eeprom struct and entire eeprom area
 * area (data) is needed for old eeprom version update and crc verification
 * because old eeprom could be bigger then current
 */
union eeprom_data {
    uint8_t data[EEPROM_MAX_DATASIZE];
    eeprom_vars_t vars;
#ifndef NO_EEPROM_UPGRADES
    struct {
        eeprom_head_t head;
        union {
            eeprom::v4::vars_body_t v4;
            eeprom::v6::vars_body_t v6;
            eeprom::v7::vars_body_t v7;
            eeprom::v9::vars_body_t v9;
            eeprom::v10::vars_body_t v10;
            eeprom::v11::vars_body_t v11;
            eeprom::v12::vars_body_t v12;
            eeprom::v32787::vars_body_t v32787;
            eeprom::v32789::vars_body_t v32789;
            eeprom::v22::vars_body_t v22;
            eeprom::current::vars_body_t current;
        };
    };
#endif // NO_EEPROM_UPGRADES
};

void eeprom_init_ram_mirror(eeprom_data &eeprom_ram_mirror) {
    CriticalSection critical_section;
    st25dv64k_user_read_bytes(EEPROM_ADDRESS, (void *)&eeprom_ram_mirror, sizeof(eeprom_ram_mirror));
}

// version independent crc32 check
bool eeprom_check_crc32(eeprom_data &eeprom_ram_mirror) {
    if (eeprom_ram_mirror.vars.head.DATASIZE > EEPROM_MAX_DATASIZE)
        return false;
    if (eeprom_ram_mirror.vars.head.DATASIZE < sizeof(eeprom_ram_mirror.vars.head) + sizeof(eeprom_ram_mirror.vars.CRC32))
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

/**
 * @brief conversion function for new version format (features, firmware version/build)
 * does not change crc, it is changed automatically by write function
 *
 * @param eevars eeprom struct in RAM
 * @return true updated (changed)
 * @return false not changed, need reset to defaults
 */
static bool eeprom_convert_from([[maybe_unused]] eeprom_data &data) {
#ifndef NO_EEPROM_UPGRADES
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
        data.v12 = eeprom::v12::convert(data.v11);
        version = 12;
    }

    if (version == 12) {
        data.v32787 = eeprom::v32787::convert(data.v12);
        version = 32787; // 19 + wrongly set bit 15
    }

    if (version == 32787) {
        data.v32789 = eeprom::v32789::convert(data.v32787);
        version = 32789; // 21 + wrongly set bit 15
    }

    if (version == 32789) {
        data.v22 = eeprom::v22::convert(data.v32789);
        version = 22;
    }

    if (version == 22) {
        data.current = eeprom::current::convert(data.v22);
        version = 23;
    }

    // after body was updated we can update head
    // don't do it before body, because it will rewrite the values in head to default values
    data.head.VERSION = EEPROM_VERSION;
    data.head.FEATURES = EEPROM_FEATURES;
    data.head.DATASIZE = EEPROM_DATASIZE;
    data.head.FWBUILD = project_build_number;

    char buffer[15];
    fill_project_version_no_dots(buffer, sizeof(buffer));
    std::from_chars(buffer, buffer + strlen(buffer), data.head.FWVERSION);

    // if update was successful, version will be current
    return version == EEPROM_VERSION;
#else  // NO_EEPROM_UPGRADES
    return false; // forces defaults
#endif // NO_EEPROM_UPGRADES
}

/**
 * @brief Migrates old eeprom into new configuration store. It is done only for one specific configuration store and only once. It is calling load_item rather than .set on a specific item directly (or obtaining the hash 'smartly') because it needs to work in case the new item gets deprecated as well (load_item handles deprecations, set doesn't)
 *
 * !!DO NOT CHANGE ANYTHING IN THIS FUNCTION UNLESS ABSOLUTELY NECESSARY!! - It should be created once, verified that it properly transfers data from old eeprom and never touched since.
 */
void migrate(::eeprom::current::vars_body_t &body) {
    // puts all of the old data into backend as one transaction

    auto guard = config_store().get_backend().transaction_guard();
    std::array<uint8_t, Journal::Backend::MAX_ITEM_SIZE> buffer {};

    auto migrate_one = [&]<typename T>(uint16_t id, const T &old_data) {
        memcpy(buffer.data(), &old_data, sizeof(T));
        config_store().get_backend().save(id, { buffer.data(), sizeof(T) });
    };

    auto migrate_str = [&]<typename T>(uint16_t id, const T &old_data, size_t length) {
        memcpy(buffer.data(), &old_data, length);
        config_store().get_backend().save(id, { buffer.data(), length });
    };

    migrate_one(Journal::hash("Run Selftest"), body.RUN_SELFTEST);
    migrate_one(Journal::hash("Run XYZ Calibration"), body.RUN_XYZCALIB);
    migrate_one(Journal::hash("Run First Layer"), body.RUN_FIRSTLAY);

    migrate_one(Journal::hash("FSensor Enabled"), body.FSENSOR_ENABLED);

    migrate_one(Journal::hash("PID Nozzle P"), body.PID_NOZ_P);
    migrate_one(Journal::hash("PID Nozzle I"), body.PID_NOZ_I);
    migrate_one(Journal::hash("PID Nozzle D"), body.PID_NOZ_D);

    migrate_one(Journal::hash("PID Bed P"), body.PID_BED_P);
    migrate_one(Journal::hash("PID Bed I"), body.PID_BED_I);
    migrate_one(Journal::hash("PID Bed D"), body.PID_BED_D);

    migrate_one(Journal::hash("LAN Flag"), body.LAN_FLAG);
    migrate_one(Journal::hash("LAN IP4 Address"), body.LAN_IP4_ADDR);
    migrate_one(Journal::hash("LAN IP4 Mask"), body.LAN_IP4_MSK);
    migrate_one(Journal::hash("LAN IP4 Gateway"), body.LAN_IP4_GW);
    migrate_one(Journal::hash("LAN IP4 DNS1"), body.LAN_IP4_DNS1);
    migrate_one(Journal::hash("LAN IP4 DNS2"), body.LAN_IP4_DNS2);
    migrate_str(Journal::hash("LAN Hostname"), body.LAN_HOSTNAME, LAN_HOSTNAME_MAX_LEN + 1);
    migrate_one(Journal::hash("LAN Timezone"), body.TIMEZONE);

    migrate_one(Journal::hash("WIFI Flag"), body.WIFI_FLAG);
    migrate_one(Journal::hash("WIFI IP4 Address"), body.WIFI_IP4_ADDR);
    migrate_one(Journal::hash("WIFI IP4 Mask"), body.WIFI_IP4_MSK);
    migrate_one(Journal::hash("WIFI IP4 Gateway"), body.WIFI_IP4_GW);
    migrate_one(Journal::hash("WIFI IP4 DNS1"), body.WIFI_IP4_DNS1);
    migrate_one(Journal::hash("WIFI IP4 DNS2"), body.WIFI_IP4_DNS2);
    migrate_str(Journal::hash("WIFI Hostname"), body.WIFI_HOSTNAME, LAN_HOSTNAME_MAX_LEN + 1);
    migrate_str(Journal::hash("WIFI AP SSID"), body.WIFI_AP_SSID, WIFI_MAX_SSID_LEN + 1);
    migrate_str(Journal::hash("WIFI AP Password"), body.WIFI_AP_PASSWD, WIFI_MAX_PASSWD_LEN + 1);

    migrate_one(Journal::hash("Sound Mode"), body.SOUND_MODE);
    migrate_one(Journal::hash("Sound Volume"), body.SOUND_VOLUME);
    migrate_one(Journal::hash("Language"), body.LANGUAGE);
    migrate_one(Journal::hash("File Sort"), body.FILE_SORT);
    migrate_one(Journal::hash("Menu Timeout"), body.MENU_TIMEOUT);
    migrate_one(Journal::hash("Devhash in QR"), body.DEVHASH_IN_QR);
    migrate_one(Journal::hash("Footer Setting"), body.FOOTER_SETTING);
    migrate_one(Journal::hash("Footer Draw Type"), body.FOOTER_DRAW_TYPE);
    migrate_one(Journal::hash("Fan Check Enabled"), body.FAN_CHECK_ENABLED);
    migrate_one(Journal::hash("FS Autoload Enabled"), body.FS_AUTOLOAD_ENABLED);
    migrate_one(Journal::hash("Odometer Time"), body.ODOMETER_TIME);
    migrate_one(Journal::hash("Active NetDev"), body.EEVAR_ACTIVE_NETDEV);
    migrate_one(Journal::hash("PrusaLink Enabled"), body.EEVAR_PL_RUN);
    migrate_str(Journal::hash("PrusaLink Password"), body.EEVAR_PL_PASSWORD, PL_PASSWORD_SIZE);

    migrate_one(Journal::hash("USB MSC Enabled"), body.USB_MSC_ENABLED);

    migrate_str(Journal::hash("Connect Host"), body.CONNECT_HOST, CONNECT_HOST_SIZE + 1);
    migrate_str(Journal::hash("Connect Token"), body.CONNECT_TOKEN, CONNECT_TOKEN_SIZE + 1);
    migrate_one(Journal::hash("Connect Port"), body.CONNECT_PORT);
    migrate_one(Journal::hash("Connect TLS"), body.CONNECT_TLS);
    migrate_one(Journal::hash("Connect Enabled"), body.CONNECT_ENABLED);

    migrate_one(Journal::hash("Job ID"), body.JOB_ID);

    migrate_one(Journal::hash("Crash Enabled"), body.CRASH_ENABLED);
    migrate_one(Journal::hash("Crash Sens X"), body.EEVAR_CRASH_SENS_X);
    migrate_one(Journal::hash("Crash Sens Y"), body.EEVAR_CRASH_SENS_Y);
    migrate_one(Journal::hash("Crash Sens Max Period X"), body.EEVAR_CRASH_MAX_PERIOD_X);
    migrate_one(Journal::hash("Crash Sens Max Period Y"), body.EEVAR_CRASH_MAX_PERIOD_Y);
    migrate_one(Journal::hash("Crash Filter"), body.EEVAR_CRASH_FILTER);
    migrate_one(Journal::hash("Crash Count X"), body.EEVAR_CRASH_COUNT_X_TOT);
    migrate_one(Journal::hash("Crash Count Y"), body.EEVAR_CRASH_COUNT_Y_TOT);
    migrate_one(Journal::hash("Power Panics Count"), body.EEVAR_POWER_COUNT_TOT);

    migrate_one(Journal::hash("Time Format"), body.TIME_FORMAT);

    migrate_one(Journal::hash("Loadcell Scale"), body.LOADCELL_SCALE);
    migrate_one(Journal::hash("Loadcell Threshold Static"), body.LOADCELL_THRS_STATIC);
    migrate_one(Journal::hash("Loadcell Hysteresis"), body.LOADCELL_HYST);
    migrate_one(Journal::hash("Loadcell Threshold Continuous"), body.LOADCELL_THRS_CONTINOUS);

    migrate_one(Journal::hash("Extruder FS Ref Value 0"), body.FS_REF_VAL_0);
    migrate_one(Journal::hash("Extruder FS Value Span 0"), body.FS_VAL_SPAN_0);
    migrate_one(Journal::hash("Extruder FS Ref Value 1"), body.FS_REF_VALUE_1);
    migrate_one(Journal::hash("Extruder FS Value Span 1"), body.FS_VALUE_SPAN_1);
    migrate_one(Journal::hash("Extruder FS Ref Value 2"), body.FS_REF_VALUE_2);
    migrate_one(Journal::hash("Extruder FS Value Span 2"), body.FS_VALUE_SPAN_2);
    migrate_one(Journal::hash("Extruder FS Ref Value 3"), body.FS_REF_VALUE_3);
    migrate_one(Journal::hash("Extruder FS Value Span 3"), body.FS_VALUE_SPAN_3);
    migrate_one(Journal::hash("Extruder FS Ref Value 4"), body.FS_REF_VALUE_4);
    migrate_one(Journal::hash("Extruder FS Value Span 4"), body.FS_VALUE_SPAN_4);
    migrate_one(Journal::hash("Extruder FS Ref Value 5"), body.FS_REF_VALUE_5);
    migrate_one(Journal::hash("Extruder FS Value Span 5"), body.FS_VALUE_SPAN_5);

    migrate_one(Journal::hash("Side FS Ref Value 0"), body.EEVAR_SIDE_FS_REF_VALUE_0);
    migrate_one(Journal::hash("Side FS Value Span 0"), body.EEVAR_SIDE_FS_VALUE_SPAN_0);
    migrate_one(Journal::hash("Side FS Ref Value 1"), body.EEVAR_SIDE_FS_REF_VALUE_1);
    migrate_one(Journal::hash("Side FS Value Span 1"), body.EEVAR_SIDE_FS_REF_VALUE_1);
    migrate_one(Journal::hash("Side FS Ref Value 2"), body.EEVAR_SIDE_FS_REF_VALUE_2);
    migrate_one(Journal::hash("Side FS Value Span 2"), body.EEVAR_SIDE_FS_VALUE_SPAN_2);
    migrate_one(Journal::hash("Side FS Ref Value 3"), body.EEVAR_SIDE_FS_REF_VALUE_3);
    migrate_one(Journal::hash("Side FS Value Span 3"), body.EEVAR_SIDE_FS_VALUE_SPAN_3);
    migrate_one(Journal::hash("Side FS Ref Value 4"), body.EEVAR_SIDE_FS_REF_VALUE_4);
    migrate_one(Journal::hash("Side FS Value Span 4"), body.EEVAR_SIDE_FS_VALUE_SPAN_4);
    migrate_one(Journal::hash("Side FS Ref Value 5"), body.EEVAR_SIDE_FS_REF_VALUE_5);
    migrate_one(Journal::hash("Side FS Value Span 5"), body.EEVAR_SIDE_FS_VALUE_SPAN_5);

    migrate_one(Journal::hash("Print Progress Time"), body.PRINT_PROGRESS_TIME);
    migrate_one(Journal::hash("TMC Wavetable Enabled"), body.TMC_WAVETABLE_ENABLE);

    migrate_one(Journal::hash("MMU2 Enabled"), body.EEVAR_MMU2_ENABLED);
    migrate_one(Journal::hash("MMU2 Cutter"), body.EEVAR_MMU2_CUTTER);
    migrate_one(Journal::hash("MMU2 Stealth Mode"), body.EEVAR_MMU2_STEALTH_MODE);

    migrate_one(Journal::hash("Run LEDs"), body.EEVAR_RUN_LEDS);
    migrate_one(Journal::hash("Heat Entire Bed"), body.HEAT_ENTIRE_BED);
    migrate_one(Journal::hash("Touch Enabled"), body.TOUCH_ENABLED);

    migrate_one(Journal::hash("Dock Position 0"), body.DOCK_POSITION_0);
    migrate_one(Journal::hash("Dock Position 1"), body.DOCK_POSITION_1);
    migrate_one(Journal::hash("Dock Position 2"), body.DOCK_POSITION_2);
    migrate_one(Journal::hash("Dock Position 3"), body.DOCK_POSITION_3);
    migrate_one(Journal::hash("Dock Position 4"), body.DOCK_POSITION_4);
    migrate_one(Journal::hash("Dock Position 5"), body.DOCK_POSITION_5);

    migrate_one(Journal::hash("Tool Offset 0"), body.TOOL_OFFSET_0);
    migrate_one(Journal::hash("Tool Offset 1"), body.TOOL_OFFSET_1);
    migrate_one(Journal::hash("Tool Offset 2"), body.TOOL_OFFSET_2);
    migrate_one(Journal::hash("Tool Offset 3"), body.TOOL_OFFSET_3);
    migrate_one(Journal::hash("Tool Offset 4"), body.TOOL_OFFSET_4);
    migrate_one(Journal::hash("Tool Offset 5"), body.TOOL_OFFSET_5);

    migrate_one(Journal::hash("Filament Type 0"), body.FILAMENT_TYPE);
    migrate_one(Journal::hash("Filament Type 1"), body.FILAMENT_TYPE_1);
    migrate_one(Journal::hash("Filament Type 2"), body.FILAMENT_TYPE_2);
    migrate_one(Journal::hash("Filament Type 3"), body.FILAMENT_TYPE_3);
    migrate_one(Journal::hash("Filament Type 4"), body.FILAMENT_TYPE_4);
    migrate_one(Journal::hash("Filament Type 5"), body.FILAMENT_TYPE_5);

    migrate_one(Journal::hash("Heatup Bed"), body.HEATUP_BED);

    migrate_one(Journal::hash("Nozzle Diameter 0"), body.NOZZLE_DIA_0);
    migrate_one(Journal::hash("Nozzle Diameter 1"), body.NOZZLE_DIA_1);
    migrate_one(Journal::hash("Nozzle Diameter 2"), body.NOZZLE_DIA_2);
    migrate_one(Journal::hash("Nozzle Diameter 3"), body.NOZZLE_DIA_3);
    migrate_one(Journal::hash("Nozzle Diameter 4"), body.NOZZLE_DIA_4);
    migrate_one(Journal::hash("Nozzle Diameter 5"), body.NOZZLE_DIA_5);

    migrate_one(Journal::hash("Homing Bump Divisor X"), body.HOMING_BDIVISOR_X);
    migrate_one(Journal::hash("Homing Bump Divisor Y"), body.HOMING_BDIVISOR_Y);

    migrate_one(Journal::hash("Enable Side LEDs"), body.EEVAR_ENABLE_SIDE_LEDS);

    migrate_one(Journal::hash("Odometer X"), body.EEVAR_ODOMETER_X);
    migrate_one(Journal::hash("Odometer Y"), body.EEVAR_ODOMETER_Y);
    migrate_one(Journal::hash("Odometer Z"), body.EEVAR_ODOMETER_Z);
    migrate_one(Journal::hash("Odometer Extruded Length 0"), body.EEVAR_ODOMETER_E0);
    migrate_one(Journal::hash("Odometer Extruded Length 1"), body.ODOMETER_E1);
    migrate_one(Journal::hash("Odometer Extruded Length 2"), body.ODOMETER_E2);
    migrate_one(Journal::hash("Odometer Extruded Length 3"), body.ODOMETER_E3);
    migrate_one(Journal::hash("Odometer Extruded Length 4"), body.ODOMETER_E4);
    migrate_one(Journal::hash("Odometer Extruded Length 5"), body.ODOMETER_E5);
    migrate_one(Journal::hash("Odometer Toolpicks 0"), body.ODOMETER_T0);
    migrate_one(Journal::hash("Odometer Toolpicks 1"), body.ODOMETER_T1);
    migrate_one(Journal::hash("Odometer Toolpicks 2"), body.ODOMETER_T2);
    migrate_one(Journal::hash("Odometer Toolpicks 3"), body.ODOMETER_T3);
    migrate_one(Journal::hash("Odometer Toolpicks 4"), body.ODOMETER_T4);
    migrate_one(Journal::hash("Odometer Toolpicks 5"), body.ODOMETER_T5);

    migrate_one(Journal::hash("HW Check Nozzle"), body.EEVAR_HWCHECK_NOZZLE);
    migrate_one(Journal::hash("HW Check Model"), body.EEVAR_HWCHECK_MODEL);
    migrate_one(Journal::hash("HW Check Firmware"), body.EEVAR_HWCHECK_FIRMW);
    migrate_one(Journal::hash("HW Check G-code"), body.EEVAR_HWCHECK_GCODE);
    migrate_one(Journal::hash("HW Check Compatibility"), body.HWCHECK_COMPATIBILITY);

    migrate_one(Journal::hash("Selftest Result V23"), body.SELFTEST_RESULT);

    migrate_one(Journal::hash("Active Sheet"), body.ACTIVE_SHEET);

    migrate_one(Journal::hash("Sheet 0"), body.SHEET_PROFILE0);
    migrate_one(Journal::hash("Sheet 1"), body.SHEET_PROFILE1);
    migrate_one(Journal::hash("Sheet 2"), body.SHEET_PROFILE2);
    migrate_one(Journal::hash("Sheet 3"), body.SHEET_PROFILE3);
    migrate_one(Journal::hash("Sheet 4"), body.SHEET_PROFILE4);
    migrate_one(Journal::hash("Sheet 5"), body.SHEET_PROFILE5);
    migrate_one(Journal::hash("Sheet 6"), body.SHEET_PROFILE6);
    migrate_one(Journal::hash("Sheet 7"), body.SHEET_PROFILE7);

    migrate_one(Journal::hash("Axis Steps Per Unit X"), body.AXIS_STEPS_PER_UNIT_X);
    migrate_one(Journal::hash("Axis Steps Per Unit Y"), body.AXIS_STEPS_PER_UNIT_Y);
    migrate_one(Journal::hash("Axis Steps Per Unit Z"), body.AXIS_STEPS_PER_UNIT_Z);
    migrate_one(Journal::hash("Axis Steps Per Unit E0"), body.AXIS_STEPS_PER_UNIT_E0);
    migrate_one(Journal::hash("Axis Microsteps X"), body.AXIS_MICROSTEPS_X);
    migrate_one(Journal::hash("Axis Microsteps Y"), body.AXIS_MICROSTEPS_Y);
    migrate_one(Journal::hash("Axis Microsteps Z"), body.AXIS_MICROSTEPS_Z);
    migrate_one(Journal::hash("Axis Microsteps E0"), body.AXIS_MICROSTEPS_E0);
    migrate_one(Journal::hash("Axis RMS Current MA X"), body.AXIS_RMS_CURRENT_MA_X);

#if PRINTER_IS_PRUSA_MK4
    // Upgrade MK4 Y current for IS only when unchanged
    migrate_one(Journal::hash("Axis RMS Current MA Y"), body.AXIS_RMS_CURRENT_MA_Y == 600 ? static_cast<uint16_t>(700) : body.AXIS_RMS_CURRENT_MA_Y);
#else
    migrate_one(Journal::hash("Axis RMS Current MA Y"), body.AXIS_RMS_CURRENT_MA_Y);
#endif

    migrate_one(Journal::hash("Axis RMS Current MA Z"), body.AXIS_RMS_CURRENT_MA_Z);
    migrate_one(Journal::hash("Axis RMS Current MA E0"), body.AXIS_RMS_CURRENT_MA_E0);
    migrate_one(Journal::hash("Axis Z Max Pos MM"), body.AXIS_Z_MAX_POS_MM);

    migrate_one(Journal::hash("Nozzle Sock"), body.NOZZLE_SOCK);
    migrate_one(Journal::hash("Nozzle Type"), body.NOZZLE_TYPE);
}
}

void init_config_store() {

    st25dv64k_init();

    old_eeprom::eeprom_data eeprom_ram_mirror;
    /* Try to read inital data from eeprom multiple times.
     * This is blind fix for random eeprom resets. We know incorrect data can
     * be read without reporting any errors. At last, this happens when read
     * is interrupted by a debugger. This bets on a silent random read error
     * and retries read after failing crc. */
    bool crc_ok = false;
    for (size_t i = 0; i < old_eeprom::EEPROM_DATA_INIT_TRIES && !crc_ok; ++i) {
        old_eeprom::eeprom_init_ram_mirror(eeprom_ram_mirror);
        crc_ok = old_eeprom::eeprom_check_crc32(eeprom_ram_mirror);
    }

    if (!crc_ok) { // old eeprom failed to start, so we're either reset or already in config_store
        config_store().init();
        config_store().load_all();
        const auto journal_state = eeprom_journal::CurrentStore::get_backend().get_journal_state();
        if (journal_state == Journal::Backend::JournalState::ColdStart) {
            config_store_init_result() = eeprom_journal::InitResult::cold_start;
        } else {
            config_store_init_result() = eeprom_journal::InitResult::normal;
        }
    } else {
        // old eeprom versions migration
        if (
            (eeprom_ram_mirror.vars.head.VERSION != old_eeprom::EEPROM_VERSION
                && eeprom_ram_mirror.vars.head.VERSION != (old_eeprom::EEPROM_VERSION ^ PRIVATE__EEPROM_OFFSET) // also accept versions with/without PRIVATE__EEPROM_OFFSET
                )
            || (eeprom_ram_mirror.vars.head.FEATURES != EEPROM_FEATURES)) {
            if (!old_eeprom::eeprom_convert_from(eeprom_ram_mirror)) {
                // nothing was converted and version doesn't match, crc was ok
                // -> weird state
                // -> load defaults (ie start config_store from zero)
                crc_ok = false;
                config_store().get_backend().erase_storage_area(); // guarantee load from nothing
                config_store().init();
                config_store_init_result() = eeprom_journal::InitResult::cold_start;
                return;
            }
        }

        // we have valid old eeprom data
        config_store_init_result() = eeprom_journal::InitResult::migrated_from_old;
        config_store().get_backend().erase_storage_area();        // guarantee load from nothing
        config_store().init();                                    // initializes the store's backend, will be a cold start
        config_store().get_backend().override_cold_start_state(); // we don't want the start to be marked as cold to load from our old eeprom transaction from migration
        old_eeprom::migrate(eeprom_ram_mirror.vars.body);         // puts all old values as one transaction into the backend
        config_store().load_all();                                // loads the config_store from the one transaction (can trigger further config_store migrations)

        // Since we have at least one migration, bank flip is guaranteed now, which will remove default value journal entries from the old eeprom migration transaction.
    }
}

void init_stores() {
    init_config_store();
}
