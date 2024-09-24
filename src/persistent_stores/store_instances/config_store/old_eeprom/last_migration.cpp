#include "last_migration.hpp"
#include <freertos/critical_section.hpp>
#include <journal/backend.hpp>
#include <charconv>
#include <version/version.hpp>
#include <config_store/backend_instance.hpp>
#include <journal/store.hpp>
#include <footer_eeprom.hpp>

namespace config_store_ns::old_eeprom {
void eeprom_init_ram_mirror(eeprom_data &eeprom_ram_mirror) {
    freertos::CriticalSection critical_section;
    st25dv64k_user_read_bytes(config_store_ns::start_address, (void *)&eeprom_ram_mirror, sizeof(eeprom_ram_mirror));
}

constexpr uint16_t EEPROM_FEATURE_PID_NOZ { 0x0001 };
constexpr uint16_t EEPROM_FEATURE_PID_BED { 0x0002 };
constexpr uint16_t EEPROM_FEATURE_LAN { 0x0004 };
constexpr uint16_t EEPROM_FEATURE_SHEETS { 0x0008 };
constexpr uint16_t EEPROM_FEATURE_LOADCELL { 0x0010 };
constexpr uint16_t EEPROM_FEATURE_MMU2 { 0x0020 };
constexpr uint16_t EEPROM_FEATURE_CONNECT { 0x0040 };

constexpr uint16_t EEPROM_FEATURES { EEPROM_FEATURE_PID_NOZ | EEPROM_FEATURE_PID_BED | EEPROM_FEATURE_LAN | EEPROM_FEATURE_LOADCELL | EEPROM_FEATURE_SHEETS | EEPROM_FEATURE_MMU2 | EEPROM_FEATURE_CONNECT };

constexpr uint16_t EEPROM_VERSION = 23;

constexpr uint16_t EEPROM_LAST_VERSION_WITH_OLD_CRC { 10 };

bool is_older_version(const eeprom_data &eeprom_ram_mirror) {
    return (eeprom_ram_mirror.vars.head.VERSION != EEPROM_VERSION)
        || (eeprom_ram_mirror.vars.head.FEATURES != EEPROM_FEATURES);
}

bool eeprom_convert_from([[maybe_unused]] eeprom_data &data) {
    uint16_t version = data.head.VERSION;
    if (version == 4) {
        data.v6 = old_eeprom::v6::convert(data.v4);
        version = 6;
    }

    if (version == 6) {
        data.v7 = old_eeprom::v7::convert(data.v6);
        version = 7;
    }

    if (version == 7) {
        data.v9 = old_eeprom::v9::convert(data.v7);
        version = 9;
    }

    if (version == 9) {
        data.v10 = old_eeprom::v10::convert(data.v9);
        version = 10;
    }

    if (version == 10) {
        data.v11 = old_eeprom::v11::convert(data.v10);
        version = 11;
    }

    if (version == 11) {
        data.v12 = old_eeprom::v12::convert(data.v11);
        version = 12;
    }

    if (version == 12) {
        data.v32787 = old_eeprom::v32787::convert(data.v12);
        version = 32787; // 19 + wrongly set bit 15
    }

    if (version == 32787) {
        data.v32789 = old_eeprom::v32789::convert(data.v32787);
        version = 32789; // 21 + wrongly set bit 15
    }

    if (version == 32789) {
        data.v22 = old_eeprom::v22::convert(data.v32789);
        version = 22;
    }

    if (version == 22) {
        data.current = old_eeprom::current::convert(data.v22);
        version = 23;
    }

    // after body was updated we can update head
    // don't do it before body, because it will rewrite the values in head to default values
    data.head.VERSION = EEPROM_VERSION;
    data.head.FEATURES = EEPROM_FEATURES;
    data.head.DATASIZE = EEPROM_DATASIZE;
    data.head.FWBUILD = version::project_build_number;

    char buffer[15];
    version::fill_project_version_no_dots(buffer, sizeof(buffer));
    std::from_chars(buffer, buffer + strlen(buffer), data.head.FWVERSION);

    // if update was successful, version will be current
    return version == EEPROM_VERSION;
}

// version independent crc32 check
bool eeprom_check_crc32(eeprom_data &eeprom_ram_mirror) {
    if (eeprom_ram_mirror.vars.head.DATASIZE > EEPROM_MAX_DATASIZE) {
        return false;
    }
    if (eeprom_ram_mirror.vars.head.DATASIZE < sizeof(eeprom_ram_mirror.vars.head) + sizeof(eeprom_ram_mirror.vars.CRC32)) {
        return false;
    }

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

void migrate(old_eeprom::current::vars_body_t &body, journal::Backend &backend) {
    // Puts all of the old data into backend as one transaction. It's done by saving new journal entries with given hashed ID (rather than setting to store directly), because it needs to work once the item gets deprecated as well
    // !!DO NOT CHANGE ANYTHING IN THIS FUNCTION UNLESS ABSOLUTELY NECESSARY!! - It should be created once, verified that it properly transfers data from old eeprom and never touched since.

    auto guard = backend.transaction_guard();
    std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> buffer {};

    auto migrate_one = [&]<typename T>(uint16_t id, const T &old_data) {
        memcpy(buffer.data(), &old_data, sizeof(T));
        backend.save(id, { buffer.data(), sizeof(T) });
    };

    auto migrate_str = [&]<typename T>(uint16_t id, const T &old_data, size_t length) {
        memcpy(buffer.data(), &old_data, length);
        backend.save(id, { buffer.data(), length });
    };

    migrate_one(journal::hash("Run Selftest"), body.RUN_SELFTEST);
    migrate_one(journal::hash("Run XYZ Calibration"), body.RUN_XYZCALIB);
    migrate_one(journal::hash("Run First Layer"), body.RUN_FIRSTLAY);

    migrate_one(journal::hash("FSensor Enabled"), body.FSENSOR_ENABLED);

    // Do not migrate PID on XL, these values were never used on XL
#if ENABLED(PIDTEMP) && !PRINTER_IS_PRUSA_XL()
    migrate_one(journal::hash("PID Nozzle P"), body.PID_NOZ_P);
    migrate_one(journal::hash("PID Nozzle I"), body.PID_NOZ_I);
    migrate_one(journal::hash("PID Nozzle D"), body.PID_NOZ_D);
#endif /* ENABLED(PIDTEMP) */

#if ENABLED(PIDTEMPBED)
    migrate_one(journal::hash("PID Bed P"), body.PID_BED_P);
    migrate_one(journal::hash("PID Bed I"), body.PID_BED_I);
    migrate_one(journal::hash("PID Bed D"), body.PID_BED_D);
#endif /* ENABLED(PIDTEMPBED) */

    migrate_one(journal::hash("LAN Flag"), body.LAN_FLAG);
    migrate_one(journal::hash("LAN IP4 Address"), body.LAN_IP4_ADDR);
    migrate_one(journal::hash("LAN IP4 Mask"), body.LAN_IP4_MSK);
    migrate_one(journal::hash("LAN IP4 Gateway"), body.LAN_IP4_GW);
    migrate_one(journal::hash("LAN IP4 DNS1"), body.LAN_IP4_DNS1);
    migrate_one(journal::hash("LAN IP4 DNS2"), body.LAN_IP4_DNS2);
    migrate_str(journal::hash("LAN Hostname"), body.LAN_HOSTNAME, old_eeprom::LAN_HOSTNAME_MAX_LEN + 1);
    migrate_one(journal::hash("LAN Timezone"), body.TIMEZONE);

    migrate_one(journal::hash("WIFI Flag"), body.WIFI_FLAG);
    migrate_one(journal::hash("WIFI IP4 Address"), body.WIFI_IP4_ADDR);
    migrate_one(journal::hash("WIFI IP4 Mask"), body.WIFI_IP4_MSK);
    migrate_one(journal::hash("WIFI IP4 Gateway"), body.WIFI_IP4_GW);
    migrate_one(journal::hash("WIFI IP4 DNS1"), body.WIFI_IP4_DNS1);
    migrate_one(journal::hash("WIFI IP4 DNS2"), body.WIFI_IP4_DNS2);
    migrate_str(journal::hash("WIFI Hostname"), body.WIFI_HOSTNAME, old_eeprom::LAN_HOSTNAME_MAX_LEN + 1);
    migrate_str(journal::hash("WIFI AP SSID"), body.WIFI_AP_SSID, old_eeprom::WIFI_MAX_SSID_LEN + 1);
    migrate_str(journal::hash("WIFI AP Password"), body.WIFI_AP_PASSWD, old_eeprom::WIFI_MAX_PASSWD_LEN + 1);

    migrate_one(journal::hash("Sound Mode"), body.SOUND_MODE);
    migrate_one(journal::hash("Sound Volume"), body.SOUND_VOLUME);
    migrate_one(journal::hash("Language"), body.LANGUAGE);
    migrate_one(journal::hash("File Sort"), body.FILE_SORT);
    migrate_one(journal::hash("Menu Timeout"), body.MENU_TIMEOUT);
    migrate_one(journal::hash("Devhash in QR"), body.DEVHASH_IN_QR);

    migrate_one(journal::hash("Footer Draw Type"), body.FOOTER_DRAW_TYPE);
    migrate_one(journal::hash("Fan Check Enabled"), body.FAN_CHECK_ENABLED);
    migrate_one(journal::hash("FS Autoload Enabled"), body.FS_AUTOLOAD_ENABLED);
    migrate_one(journal::hash("Odometer Time"), body.ODOMETER_TIME);
    migrate_one(journal::hash("Active NetDev"), body.EEVAR_ACTIVE_NETDEV);
    migrate_one(journal::hash("PrusaLink Enabled"), body.EEVAR_PL_RUN);
    migrate_str(journal::hash("PrusaLink Password"), body.EEVAR_PL_PASSWORD, old_eeprom::PL_PASSWORD_SIZE);

    migrate_one(journal::hash("USB MSC Enabled"), body.USB_MSC_ENABLED);

    migrate_str(journal::hash("Connect Host"), body.CONNECT_HOST, old_eeprom::CONNECT_HOST_SIZE + 1);
    migrate_str(journal::hash("Connect Token"), body.CONNECT_TOKEN, old_eeprom::CONNECT_TOKEN_SIZE + 1);
    migrate_one(journal::hash("Connect Port"), body.CONNECT_PORT);
    migrate_one(journal::hash("Connect TLS"), body.CONNECT_TLS);
    migrate_one(journal::hash("Connect Enabled"), body.CONNECT_ENABLED);

    migrate_one(journal::hash("Job ID"), body.JOB_ID);

    migrate_one(journal::hash("Crash Enabled"), body.CRASH_ENABLED);
    migrate_one(journal::hash("Crash Sens X"), body.EEVAR_CRASH_SENS_X);
    migrate_one(journal::hash("Crash Sens Y"), body.EEVAR_CRASH_SENS_Y);
    migrate_one(journal::hash("Crash Sens Max Period X"), body.EEVAR_CRASH_MAX_PERIOD_X);
    migrate_one(journal::hash("Crash Sens Max Period Y"), body.EEVAR_CRASH_MAX_PERIOD_Y);
    migrate_one(journal::hash("Crash Filter"), body.EEVAR_CRASH_FILTER);
    migrate_one(journal::hash("Crash Count X"), body.EEVAR_CRASH_COUNT_X_TOT);
    migrate_one(journal::hash("Crash Count Y"), body.EEVAR_CRASH_COUNT_Y_TOT);
    migrate_one(journal::hash("Power Panics Count"), body.EEVAR_POWER_COUNT_TOT);

    migrate_one(journal::hash("Time Format"), body.TIME_FORMAT);

    migrate_one(journal::hash("Loadcell Scale"), body.LOADCELL_SCALE);
    migrate_one(journal::hash("Loadcell Threshold Static"), body.LOADCELL_THRS_STATIC);
    migrate_one(journal::hash("Loadcell Hysteresis"), body.LOADCELL_HYST);
    migrate_one(journal::hash("Loadcell Threshold Continuous"), body.LOADCELL_THRS_CONTINOUS);

    migrate_one(journal::hash("Extruder FS Ref Value 0"), body.FS_REF_VAL_0);
    migrate_one(journal::hash("Extruder FS Value Span 0"), body.FS_VAL_SPAN_0);
    migrate_one(journal::hash("Extruder FS Ref Value 1"), body.FS_REF_VALUE_1);
    migrate_one(journal::hash("Extruder FS Value Span 1"), body.FS_VALUE_SPAN_1);
    migrate_one(journal::hash("Extruder FS Ref Value 2"), body.FS_REF_VALUE_2);
    migrate_one(journal::hash("Extruder FS Value Span 2"), body.FS_VALUE_SPAN_2);
    migrate_one(journal::hash("Extruder FS Ref Value 3"), body.FS_REF_VALUE_3);
    migrate_one(journal::hash("Extruder FS Value Span 3"), body.FS_VALUE_SPAN_3);
    migrate_one(journal::hash("Extruder FS Ref Value 4"), body.FS_REF_VALUE_4);
    migrate_one(journal::hash("Extruder FS Value Span 4"), body.FS_VALUE_SPAN_4);
    migrate_one(journal::hash("Extruder FS Ref Value 5"), body.FS_REF_VALUE_5);
    migrate_one(journal::hash("Extruder FS Value Span 5"), body.FS_VALUE_SPAN_5);

    migrate_one(journal::hash("Side FS Ref Value 0"), body.EEVAR_SIDE_FS_REF_VALUE_0);
    migrate_one(journal::hash("Side FS Value Span 0"), body.EEVAR_SIDE_FS_VALUE_SPAN_0);
    migrate_one(journal::hash("Side FS Ref Value 1"), body.EEVAR_SIDE_FS_REF_VALUE_1);
    migrate_one(journal::hash("Side FS Value Span 1"), body.EEVAR_SIDE_FS_REF_VALUE_1);
    migrate_one(journal::hash("Side FS Ref Value 2"), body.EEVAR_SIDE_FS_REF_VALUE_2);
    migrate_one(journal::hash("Side FS Value Span 2"), body.EEVAR_SIDE_FS_VALUE_SPAN_2);
    migrate_one(journal::hash("Side FS Ref Value 3"), body.EEVAR_SIDE_FS_REF_VALUE_3);
    migrate_one(journal::hash("Side FS Value Span 3"), body.EEVAR_SIDE_FS_VALUE_SPAN_3);
    migrate_one(journal::hash("Side FS Ref Value 4"), body.EEVAR_SIDE_FS_REF_VALUE_4);
    migrate_one(journal::hash("Side FS Value Span 4"), body.EEVAR_SIDE_FS_VALUE_SPAN_4);
    migrate_one(journal::hash("Side FS Ref Value 5"), body.EEVAR_SIDE_FS_REF_VALUE_5);
    migrate_one(journal::hash("Side FS Value Span 5"), body.EEVAR_SIDE_FS_VALUE_SPAN_5);

    migrate_one(journal::hash("Print Progress Time"), body.PRINT_PROGRESS_TIME);
    migrate_one(journal::hash("TMC Wavetable Enabled"), body.TMC_WAVETABLE_ENABLE);

    migrate_one(journal::hash("MMU2 Enabled"), body.EEVAR_MMU2_ENABLED);
    migrate_one(journal::hash("MMU2 Cutter"), body.EEVAR_MMU2_CUTTER);
    migrate_one(journal::hash("MMU2 Stealth Mode"), body.EEVAR_MMU2_STEALTH_MODE);

    migrate_one(journal::hash("Run LEDs"), body.EEVAR_RUN_LEDS);
    migrate_one(journal::hash("Heat Entire Bed"), body.HEAT_ENTIRE_BED);
    migrate_one(journal::hash("Touch Enabled"), body.TOUCH_ENABLED);

    migrate_one(journal::hash("Dock Position 0"), body.DOCK_POSITION_0);
    migrate_one(journal::hash("Dock Position 1"), body.DOCK_POSITION_1);
    migrate_one(journal::hash("Dock Position 2"), body.DOCK_POSITION_2);
    migrate_one(journal::hash("Dock Position 3"), body.DOCK_POSITION_3);
    migrate_one(journal::hash("Dock Position 4"), body.DOCK_POSITION_4);
    migrate_one(journal::hash("Dock Position 5"), body.DOCK_POSITION_5);

    migrate_one(journal::hash("Tool Offset 0"), body.TOOL_OFFSET_0);
    migrate_one(journal::hash("Tool Offset 1"), body.TOOL_OFFSET_1);
    migrate_one(journal::hash("Tool Offset 2"), body.TOOL_OFFSET_2);
    migrate_one(journal::hash("Tool Offset 3"), body.TOOL_OFFSET_3);
    migrate_one(journal::hash("Tool Offset 4"), body.TOOL_OFFSET_4);
    migrate_one(journal::hash("Tool Offset 5"), body.TOOL_OFFSET_5);

    migrate_one(journal::hash("Filament Type 0"), body.FILAMENT_TYPE);
    migrate_one(journal::hash("Filament Type 1"), body.FILAMENT_TYPE_1);
    migrate_one(journal::hash("Filament Type 2"), body.FILAMENT_TYPE_2);
    migrate_one(journal::hash("Filament Type 3"), body.FILAMENT_TYPE_3);
    migrate_one(journal::hash("Filament Type 4"), body.FILAMENT_TYPE_4);
    migrate_one(journal::hash("Filament Type 5"), body.FILAMENT_TYPE_5);

    migrate_one(journal::hash("Heatup Bed"), body.HEATUP_BED);

    migrate_one(journal::hash("Nozzle Diameter 0"), body.NOZZLE_DIA_0);
    migrate_one(journal::hash("Nozzle Diameter 1"), body.NOZZLE_DIA_1);
    migrate_one(journal::hash("Nozzle Diameter 2"), body.NOZZLE_DIA_2);
    migrate_one(journal::hash("Nozzle Diameter 3"), body.NOZZLE_DIA_3);
    migrate_one(journal::hash("Nozzle Diameter 4"), body.NOZZLE_DIA_4);
    migrate_one(journal::hash("Nozzle Diameter 5"), body.NOZZLE_DIA_5);

    migrate_one(journal::hash("Homing Bump Divisor X"), body.HOMING_BDIVISOR_X);
    migrate_one(journal::hash("Homing Bump Divisor Y"), body.HOMING_BDIVISOR_Y);

    migrate_one(journal::hash("Enable Side LEDs"), body.EEVAR_ENABLE_SIDE_LEDS);

    migrate_one(journal::hash("Odometer X"), body.EEVAR_ODOMETER_X);
    migrate_one(journal::hash("Odometer Y"), body.EEVAR_ODOMETER_Y);
    migrate_one(journal::hash("Odometer Z"), body.EEVAR_ODOMETER_Z);
    migrate_one(journal::hash("Odometer Extruded Length 0"), body.EEVAR_ODOMETER_E0);
    migrate_one(journal::hash("Odometer Extruded Length 1"), body.ODOMETER_E1);
    migrate_one(journal::hash("Odometer Extruded Length 2"), body.ODOMETER_E2);
    migrate_one(journal::hash("Odometer Extruded Length 3"), body.ODOMETER_E3);
    migrate_one(journal::hash("Odometer Extruded Length 4"), body.ODOMETER_E4);
    migrate_one(journal::hash("Odometer Extruded Length 5"), body.ODOMETER_E5);
    migrate_one(journal::hash("Odometer Toolpicks 0"), body.ODOMETER_T0);
    migrate_one(journal::hash("Odometer Toolpicks 1"), body.ODOMETER_T1);
    migrate_one(journal::hash("Odometer Toolpicks 2"), body.ODOMETER_T2);
    migrate_one(journal::hash("Odometer Toolpicks 3"), body.ODOMETER_T3);
    migrate_one(journal::hash("Odometer Toolpicks 4"), body.ODOMETER_T4);
    migrate_one(journal::hash("Odometer Toolpicks 5"), body.ODOMETER_T5);

    migrate_one(journal::hash("HW Check Nozzle"), body.EEVAR_HWCHECK_NOZZLE);
    migrate_one(journal::hash("HW Check Model"), body.EEVAR_HWCHECK_MODEL);
    migrate_one(journal::hash("HW Check Firmware"), body.EEVAR_HWCHECK_FIRMW);
    migrate_one(journal::hash("HW Check G-code"), body.EEVAR_HWCHECK_GCODE);
    migrate_one(journal::hash("HW Check Compatibility"), body.HWCHECK_COMPATIBILITY);

    migrate_one(journal::hash("Selftest Result V23"), body.SELFTEST_RESULT);

    migrate_one(journal::hash("Active Sheet"), body.ACTIVE_SHEET);

    migrate_one(journal::hash("Sheet 0"), body.SHEET_PROFILE0);
    migrate_one(journal::hash("Sheet 1"), body.SHEET_PROFILE1);
    migrate_one(journal::hash("Sheet 2"), body.SHEET_PROFILE2);
    migrate_one(journal::hash("Sheet 3"), body.SHEET_PROFILE3);
    migrate_one(journal::hash("Sheet 4"), body.SHEET_PROFILE4);
    migrate_one(journal::hash("Sheet 5"), body.SHEET_PROFILE5);
    migrate_one(journal::hash("Sheet 6"), body.SHEET_PROFILE6);
    migrate_one(journal::hash("Sheet 7"), body.SHEET_PROFILE7);

    migrate_one(journal::hash("Axis Steps Per Unit X"), body.AXIS_STEPS_PER_UNIT_X);
    migrate_one(journal::hash("Axis Steps Per Unit Y"), body.AXIS_STEPS_PER_UNIT_Y);
    migrate_one(journal::hash("Axis Steps Per Unit Z"), body.AXIS_STEPS_PER_UNIT_Z);
    migrate_one(journal::hash("Axis Steps Per Unit E0"), body.AXIS_STEPS_PER_UNIT_E0);
    migrate_one(journal::hash("Axis Microsteps X"), body.AXIS_MICROSTEPS_X);
    migrate_one(journal::hash("Axis Microsteps Y"), body.AXIS_MICROSTEPS_Y);
    migrate_one(journal::hash("Axis Microsteps Z"), body.AXIS_MICROSTEPS_Z);
    migrate_one(journal::hash("Axis Microsteps E0"), body.AXIS_MICROSTEPS_E0);
    migrate_one(journal::hash("Axis RMS Current MA X"), body.AXIS_RMS_CURRENT_MA_X);

#if PRINTER_IS_PRUSA_MK4()
    // Upgrade MK4 Y current for IS only when unchanged
    migrate_one(journal::hash("Axis RMS Current MA Y"), body.AXIS_RMS_CURRENT_MA_Y == 600 ? static_cast<uint16_t>(700) : body.AXIS_RMS_CURRENT_MA_Y);
#else
    migrate_one(journal::hash("Axis RMS Current MA Y"), body.AXIS_RMS_CURRENT_MA_Y);
#endif

    migrate_one(journal::hash("Axis RMS Current MA Z"), body.AXIS_RMS_CURRENT_MA_Z);
    migrate_one(journal::hash("Axis RMS Current MA E0"), body.AXIS_RMS_CURRENT_MA_E0);
    migrate_one(journal::hash("Axis Z Max Pos MM"), body.AXIS_Z_MAX_POS_MM);

    migrate_one(journal::hash("Nozzle Sock"), body.NOZZLE_SOCK);
    migrate_one(journal::hash("Nozzle Type"), body.NOZZLE_TYPE);

#if (PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_XL()) && HAS_GUI()
    {
        auto decoded_rec = footer::eeprom::decode_from_old_eeprom_v22(body.FOOTER_SETTING);

        migrate_one(journal::hash("Footer Setting 0"), decoded_rec[0]);
    #if FOOTER_ITEMS_PER_LINE__ > 1
        migrate_one(journal::hash("Footer Setting 1"), decoded_rec[1]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 2
        migrate_one(journal::hash("Footer Setting 2"), decoded_rec[2]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 3
        migrate_one(journal::hash("Footer Setting 3"), decoded_rec[3]);
    #endif
    #if FOOTER_ITEMS_PER_LINE__ > 4
        migrate_one(journal::hash("Footer Setting 4"), decoded_rec[4]);
    #endif
    }
#endif
}

} // namespace config_store_ns::old_eeprom
