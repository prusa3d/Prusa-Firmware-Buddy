#pragma once
#include <Marlin/src/inc/MarlinConfigPre.h>

#include <bitset>

#include "constants.hpp"
#include "defaults.hpp"
#include <option/has_config_store_wo_backend.h>
#if HAS_CONFIG_STORE_WO_BACKEND()
    #include <no_backend/store.hpp>
#else
    #include <journal/store.hpp>
    #include "backend_instance.hpp"
#endif
#include <Marlin/src/feature/input_shaper/input_shaper_config.hpp>
#include <module/temperature.h>
#include <config.h>
#include <sound_enum.h>
#include <footer_eeprom.hpp>
#include <time_tools.hpp>
#include <encoded_filament.hpp>
#include <selftest_result.hpp>
#include <module/prusa/dock_position.hpp>
#include <module/prusa/tool_offset.hpp>
#include <filament_sensors_remap_data.hpp>
#include <tristate.hpp>
#include <option/has_loadcell.h>
#include <option/has_sheet_profiles.h>
#include <option/has_adc_side_fsensor.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#include <option/has_selftest.h>
#include <option/has_phase_stepping.h>
#include <option/has_phase_stepping_toggle.h>
#include <option/has_i2c_expander.h>
#include <option/has_xbuddy_extension.h>
#include <option/has_emergency_stop.h>
#include <option/has_side_leds.h>
#include <option/xl_enclosure_support.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_precise_homing.h>
#include <option/has_chamber_filtration_api.h>
#include <option/has_auto_retract.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_chamber_vents.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_manual_belt_tuning.h>
#include <common/extended_printer_type.hpp>
#include <common/hw_check.hpp>
#include <pwm_utils.hpp>
#include <feature/xbuddy_extension/xbuddy_extension_fan_results.hpp>
#include <print_fan_type.hpp>

#if HAS_SHEET_PROFILES()
    #include <common/sheet.hpp>
#endif

#if HAS_PRECISE_HOMING_COREXY()
    #include <Marlin/src/module/prusa/homing_corexy_config.hpp>
#endif

#include <option/has_chamber_filtration_api.h>
#if HAS_CHAMBER_FILTRATION_API()
    #include <feature/chamber_filtration/chamber_filtration_enums.hpp>
#endif

#include <option/has_chamber_vents.h>
#if HAS_CHAMBER_VENTS()
    #include <feature/chamber/chamber_enums.hpp>
#endif

#include <option/has_hotend_type_support.h>
#if HAS_HOTEND_TYPE_SUPPORT()
    #include <hotend_type.hpp>
#endif

#if HAS_SIDE_LEDS()
    #include "leds/dimming_enabled.hpp"
#endif

namespace config_store_ns {

struct ItemFlag {
    using ItemFlags = journal::ItemFlags;

    static constexpr ItemFlags none = 0;

    /// Results of selftests and calibrations.
    static constexpr ItemFlags calibrations = 1 << 0;

    /// Things that can sneakily screw up the printer when they are changed.
    /// These items are to be cleared first if anything is wrong with the printer.
    /// ! This is a "flag" - no item should have this category only
    static constexpr ItemFlags common_misconfigurations = 1 << 1;

    /// Network configuration items.
    static constexpr ItemFlags network = 1 << 2;

    /// User interface related items, do not affect printer functionality.
    static constexpr ItemFlags user_interface = 1 << 3;

    /// Printer statistic.
    static constexpr ItemFlags stats = 1 << 4;

    /// Configuration of the hardware (printer type, extruder type, ...)
    static constexpr ItemFlags hw_config = 1 << 5;

    /// What filaments are currently loaded, what steel sheet is selected, ...
    static constexpr ItemFlags printer_state = 1 << 6;

    /// User filament profiles, sheet profiles, ...
    static constexpr ItemFlags user_presets = 1 << 7;

    /// Non-essential features/functionality of the printers
    static constexpr ItemFlags features = 1 << 8;

    /// Items that are dev only and are not even configurable in the production FW
    /// Quite similar to common_misconfigurations for the use cases
    static constexpr ItemFlags dev_items = 1 << 9;

    /// Special items, completely outside of categorization and selective factory reset, that have a specific handling
    static constexpr ItemFlags special = 1 << 10;
}; // namespace ItemFlag

/**
 * @brief Holds all current store items -> there is a RAM mirror of this data which is loaded upon device restart from eeprom.

 * !! HASHES CANNOT BE CHANGED !!
 * This HASH cannot be the same as an already existing one (there is a compiler check to ensure this).
 * !! NEVER JUST DELETE AN ITEM FROM THIS STRUCT; if an item is no longer wanted, deprecate it. See DeprecatedStore (below).
 * !! Changing DEFAULT VALUE is ALSO a deprecation !!
 */

struct CurrentStore
#if HAS_CONFIG_STORE_WO_BACKEND()
    : public no_backend::NBJournalCurrentStoreConfig
#else
    : public journal::CurrentStoreConfig<journal::Backend, backend>
#endif
{
    /// Performs a check on the configuration
    /// This is an opportunity to check for invalid config combinations and such
    void perform_config_check();

    /// Config store "version", gets incremented each time we need to add a new config migration
    static constexpr uint8_t newest_config_version = 5;

    /// Stores newest_migration_version of the previous firmware
    StoreItem<uint8_t, 0, ItemFlag::special, journal::hash("Config Version")> config_version;

    // wizard flags
    StoreItem<bool, true, ItemFlag::calibrations, journal::hash("Run Selftest")> run_selftest;

    /// If false, a ScreenPrinterSetup will appear on printer boot
    StoreItem<bool, false, ItemFlag::network, journal::hash("Printer network done")> printer_network_setup_done;
    StoreItem<bool, false, ItemFlag::hw_config, journal::hash("Printer hw-config done")> printer_hw_config_done;

    /// Global filament sensor enable
    StoreItem<bool, defaults::fsensor_enabled, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("FSensor Enabled V2")> fsensor_enabled;

    /// BFW-5545 When filament sensor is not responding during filament change, the user has an option to disable it.
    /// This is a flag to remind them to turn it back on again when they finis printing
    StoreItem<bool, false, ItemFlag::printer_state, journal::hash("Show Fsensors Disabled warning after print")> show_fsensors_disabled_warning_after_print;

    /// Bitfield of enabled side filament sensors
    StoreItem<uint8_t, 0xff, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("Extruder FSensors enabled")> fsensor_side_enabled_bits;

    /// Bitfield of enabled toolhead filament sensors
    StoreItem<uint8_t, 0xff, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("Side FSensors enabled")> fsensor_extruder_enabled_bits;

    // nozzle PID variables
    StoreItem<float, defaults::pid_nozzle_p, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("PID Nozzle P")> pid_nozzle_p;
    StoreItem<float, defaults::pid_nozzle_i, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("PID Nozzle I")> pid_nozzle_i;
    StoreItem<float, defaults::pid_nozzle_d, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("PID Nozzle D")> pid_nozzle_d;

    // bed PID variables
    StoreItem<float, defaults::pid_bed_p, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("PID Bed P")> pid_bed_p;
    StoreItem<float, defaults::pid_bed_i, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("PID Bed I")> pid_bed_i;
    StoreItem<float, defaults::pid_bed_d, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("PID Bed D")> pid_bed_d;

    // LAN settings
    // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1
    StoreItem<uint8_t, 0, ItemFlag::network, journal::hash("LAN Flag")> lan_flag;
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("LAN IP4 Address")> lan_ip4_addr; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("LAN IP4 Mask")> lan_ip4_mask; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("LAN IP4 Gateway")> lan_ip4_gateway; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("LAN IP4 DNS1")> lan_ip4_dns1; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("LAN IP4 DNS2")> lan_ip4_dns2; // X.X.X.X address encoded

    StoreItem<int8_t, defaults::lan_timezone, ItemFlag::user_interface, journal::hash("LAN Timezone")> timezone; // hour difference from UTC
    StoreItem<time_tools::TimezoneOffsetMinutes, defaults::timezone_minutes, ItemFlag::user_interface, journal::hash("Timezone Minutes")> timezone_minutes; // minutes offset for hour difference from UTC
    StoreItem<time_tools::TimezoneOffsetSummerTime, defaults::timezone_summer, ItemFlag::user_interface, journal::hash("Timezone Summertime")> timezone_summer; // Summertime hour offset

    // WIFI settings
    // wifi_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1, wifi_flag & 0b1100 -> reserved, previously ap_sec_t security
    StoreItem<uint8_t, 0, ItemFlag::network, journal::hash("WIFI Flag")> wifi_flag;
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("WIFI IP4 Address")> wifi_ip4_addr; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("WIFI IP4 Mask")> wifi_ip4_mask; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("WIFI IP4 Gateway")> wifi_ip4_gateway; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("WIFI IP4 DNS1")> wifi_ip4_dns1; // X.X.X.X address encoded
    StoreItem<uint32_t, 0, ItemFlag::network, journal::hash("WIFI IP4 DNS2")> wifi_ip4_dns2; // X.X.X.X address encoded
    StoreItem<std::array<char, wifi_max_ssid_len + 1>, defaults::wifi_ap_ssid, ItemFlag::network, journal::hash("WIFI AP SSID")> wifi_ap_ssid;
    StoreItem<std::array<char, wifi_max_passwd_len + 1>, defaults::wifi_ap_password, ItemFlag::network, journal::hash("WIFI AP Password")> wifi_ap_password;

    // General network settings
    StoreItem<std::array<char, lan_hostname_max_len + 1>, defaults::net_hostname, ItemFlag::network, journal::hash("Hostname")> hostname;

    StoreItem<eSOUND_MODE, defaults::sound_mode, ItemFlag::user_interface, journal::hash("Sound Mode")> sound_mode;
    StoreItem<uint8_t, defaults::sound_volume, ItemFlag::user_interface, journal::hash("Sound Volume")> sound_volume;
    StoreItem<uint16_t, defaults::language, ItemFlag::user_interface, journal::hash("Language")> language;
    StoreItem<uint8_t, 0, ItemFlag::user_interface, journal::hash("File Sort")> file_sort; // filebrowser file sort options
#if not PRINTER_IS_PRUSA_MINI()
    StoreItem<bool, true, ItemFlag::user_interface, journal::hash("Show File Extensions")> show_file_extensions; // file extension visibility
#endif
    StoreItem<bool, true, ItemFlag::user_interface, journal::hash("Menu Timeout")> menu_timeout; // on / off menu timeout flag
    StoreItem<bool, true, ItemFlag::user_interface, journal::hash("Devhash in QR")> devhash_in_qr; // on / off sending UID in QR

    StoreItem<footer::Item, defaults::footer_setting_0, ItemFlag::user_interface, journal::hash("Footer Setting 0 v3")> footer_setting_0;
#if FOOTER_ITEMS_PER_LINE__ > 1
    StoreItem<footer::Item, defaults::footer_setting_1, ItemFlag::user_interface, journal::hash("Footer Setting 1 v3")> footer_setting_1;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    StoreItem<footer::Item, defaults::footer_setting_2, ItemFlag::user_interface, journal::hash("Footer Setting 2 v3")> footer_setting_2;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    StoreItem<footer::Item, defaults::footer_setting_3, ItemFlag::user_interface, journal::hash("Footer Setting 3 v3")> footer_setting_3;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    StoreItem<footer::Item, defaults::footer_setting_4, ItemFlag::user_interface, journal::hash("Footer Setting 4 v3")> footer_setting_4;
#endif

    footer::Item get_footer_setting(uint8_t index);
    void set_footer_setting(uint8_t index, footer::Item value);

    StoreItem<uint32_t, defaults::footer_draw_type, ItemFlag::user_interface, journal::hash("Footer Draw Type")> footer_draw_type;
    StoreItem<bool, true, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("Fan Check Enabled")> fan_check_enabled;
    StoreItem<bool, true, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("FS Autoload Enabled")> fs_autoload_enabled;

    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Odometer Time")> odometer_time;
    StoreItem<uint8_t, 0, ItemFlag::network, journal::hash("Active NetDev")> active_netdev; // active network device
    StoreItem<bool, true, ItemFlag::network, journal::hash("PrusaLink Enabled")> prusalink_enabled;
    StoreItem<std::array<char, pl_password_size>, defaults::prusalink_password, ItemFlag::network, journal::hash("PrusaLink Password")> prusalink_password;

    StoreItem<std::array<char, connect_host_size + 1>, defaults::connect_host, ItemFlag::network | ItemFlag::dev_items, journal::hash("Connect Host")> connect_host;
    StoreItem<std::array<char, connect_token_size + 1>, defaults::connect_token, ItemFlag::network, journal::hash("Connect Token")> connect_token;
    StoreItem<std::array<char, connect_proxy_size + 1>, defaults::connect_proxy_host, ItemFlag::network | ItemFlag::dev_items, journal::hash("Connect Proxy Host")> connect_proxy_host;
    StoreItem<uint16_t, defaults::connect_port, ItemFlag::network | ItemFlag::dev_items, journal::hash("Connect Port")> connect_port;
    StoreItem<uint16_t, 0, ItemFlag::network | ItemFlag::dev_items, journal::hash("Connect proxy port")> connect_proxy_port;
    StoreItem<bool, true, ItemFlag::network | ItemFlag::dev_items, journal::hash("Connect TLS")> connect_tls;
    StoreItem<bool, false, ItemFlag::network, journal::hash("Connect Enabled")> connect_enabled;
    StoreItem<bool, false, ItemFlag::network | ItemFlag::dev_items, journal::hash("Connect custom TLS certificate")> connect_custom_tls_cert;

    // Metrics
    StoreItem<bool, defaults::enable_metrics, ItemFlag::network, journal::hash("Metrics Init")> enable_metrics;
    StoreItem<std::array<char, metrics_host_size + 1>, defaults::metrics_host, ItemFlag::network, journal::hash("Metrics Host")> metrics_host;
    StoreItem<uint16_t, 8514, ItemFlag::network, journal::hash("Metrics Port")> metrics_port; ///< Port used to allow and init metrics
    StoreItem<uint16_t, 13514, ItemFlag::network, journal::hash("Log Port")> syslog_port; ///< Port used to allow and init log (uses metrics_host)

    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("Job ID")> job_id; // print job id incremented at every print start

    StoreItem<bool, defaults::crash_enabled, ItemFlag::features, journal::hash("Crash Enabled")> crash_enabled;
    StoreItem<int16_t, defaults::crash_sens_x, ItemFlag::features | ItemFlag::dev_items, journal::hash("Crash Sens X")> crash_sens_x; // X axis crash sensitivity
    StoreItem<int16_t, defaults::crash_sens_y, ItemFlag::features | ItemFlag::dev_items, journal::hash("Crash Sens Y")> crash_sens_y; // Y axis crash sensitivity

    // X axis max crash period (speed) threshold
    StoreItem<uint16_t, defaults::crash_max_period_x, ItemFlag::features | ItemFlag::dev_items, journal::hash("Crash Sens Max Period X")> crash_max_period_x;
    // Y axis max crash period (speed) threshold
    StoreItem<uint16_t, defaults::crash_max_period_y, ItemFlag::features | ItemFlag::dev_items, journal::hash("Crash Sens Max Period Y")> crash_max_period_y;
    StoreItem<bool, defaults::crash_filter, ItemFlag::features | ItemFlag::dev_items, journal::hash("Crash Filter")> crash_filter; // Stallguard filtration
    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("Crash Count X")> crash_count_x; // number of crashes of X axis in total
    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("Crash Count Y")> crash_count_y; // number of crashes of Y axis in total
    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("Power Panics Count")> power_panics_count; // number of power losses in total

    StoreItem<time_tools::TimeFormat, defaults::time_format, ItemFlag::user_interface, journal::hash("Time Format")> time_format;

    // filament sensor values:
    // ref value: value of filament sensor in moment of calibration (w/o filament present)
    // value span: minimal difference of raw values between the two states of the filament sensor
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Extruder FS Ref Value 0")> extruder_fs_ref_nins_value_0;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Extruder FS INS Ref Value 0")> extruder_fs_ref_ins_value_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Extruder FS Ref Value 1")> extruder_fs_ref_nins_value_1;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Extruder FS INS Ref Value 1")> extruder_fs_ref_ins_value_1;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Extruder FS Ref Value 2")> extruder_fs_ref_nins_value_2;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Extruder FS INS Ref Value 2")> extruder_fs_ref_ins_value_2;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Extruder FS Ref Value 3")> extruder_fs_ref_nins_value_3;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Extruder FS INS Ref Value 3")> extruder_fs_ref_ins_value_3;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Extruder FS Ref Value 4")> extruder_fs_ref_nins_value_4;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Extruder FS INS Ref Value 4")> extruder_fs_ref_ins_value_4;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Extruder FS Ref Value 5")> extruder_fs_ref_nins_value_5;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Extruder FS INS Ref Value 5")> extruder_fs_ref_ins_value_5;
#endif

#if HAS_ADC_SIDE_FSENSOR() // for now not ifdefing per-extruder as well for simplicity
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Side FS Ref Value 0")> side_fs_ref_nins_value_0;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Side FS Ref INS Value 0")> side_fs_ref_ins_value_0;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Side FS Ref Value 1")> side_fs_ref_nins_value_1;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Side FS Ref INS Value 1")> side_fs_ref_ins_value_1;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Side FS Ref Value 2")> side_fs_ref_nins_value_2;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Side FS Ref INS Value 2")> side_fs_ref_ins_value_2;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Side FS Ref Value 3")> side_fs_ref_nins_value_3;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Side FS Ref INS Value 3")> side_fs_ref_ins_value_3;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Side FS Ref Value 4")> side_fs_ref_nins_value_4;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Side FS Ref INS Value 4")> side_fs_ref_ins_value_4;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, ItemFlag::calibrations, journal::hash("Side FS Ref Value 5")> side_fs_ref_nins_value_5;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, ItemFlag::calibrations, journal::hash("Side FS Ref INS Value 5")> side_fs_ref_ins_value_5;
#endif

#if HAS_MMU2()
    StoreItem<bool, false, ItemFlag::hw_config, journal::hash("Is MMU Rework")> is_mmu_rework; // Indicates printer has been reworked for MMU (has a different FS behavior)
#endif

    StoreItem<side_fsensor_remap::Mapping, defaults::side_fs_remap, ItemFlag::hw_config, journal::hash("Side FS Remap")> side_fs_remap; ///< Side filament sensor remapping

    //// Helper array-like access functions for filament sensors
    int32_t get_extruder_fs_ref_nins_value(uint8_t index);
    int32_t get_extruder_fs_ref_ins_value(uint8_t index);
    void set_extruder_fs_ref_nins_value(uint8_t index, int32_t value);
    void set_extruder_fs_ref_ins_value(uint8_t index, int32_t value);

#if HAS_ADC_SIDE_FSENSOR()
    int32_t get_side_fs_ref_nins_value(uint8_t index);
    int32_t get_side_fs_ref_ins_value(uint8_t index);
    void set_side_fs_ref_nins_value(uint8_t index, int32_t value);
    void set_side_fs_ref_ins_value(uint8_t index, int32_t value);
#endif

    StoreItem<uint16_t, defaults::print_progress_time, ItemFlag::user_interface, journal::hash("Print Progress Time")> print_progress_time; // screen progress time in seconds
    StoreItem<bool, true, ItemFlag::hw_config | ItemFlag::dev_items, journal::hash("TMC Wavetable Enabled")> tmc_wavetable_enabled; // wavetable in TMC drivers

#if HAS_MMU2()
    StoreItem<bool, false, ItemFlag::features, journal::hash("MMU2 Enabled")> mmu2_enabled;
    StoreItem<bool, false, ItemFlag::features | ItemFlag::hw_config, journal::hash("MMU2 Cutter")> mmu2_cutter; // use MMU2 cutter when it sees fit
    StoreItem<bool, false, ItemFlag::features, journal::hash("MMU2 Stealth Mode")> mmu2_stealth_mode; // run MMU2 in stealth mode wherever possible

    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("MMU2 load fails")> mmu2_load_fails;
    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("MMU2 total load fails")> mmu2_total_load_fails;
    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("MMU2 general fails")> mmu2_fails;
    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("MMU2 total general fails")> mmu2_total_fails;
#endif
    // Should we verify gcode (CRC & similar)?
    StoreItem<bool, true, ItemFlag::features, journal::hash("Verify Gcode")> verify_gcode;

    StoreItem<bool, true, ItemFlag::user_interface, journal::hash("Run LEDs")> run_leds;
    StoreItem<bool, false, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("Heat Entire Bed")> heat_entire_bed;
    StoreItem<bool, false, ItemFlag::user_interface, journal::hash("Touch Enabled")> touch_enabled;
    StoreItem<bool, false, ItemFlag::user_interface | ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Touch Sig Workaround")> touch_sig_workaround;

#if HAS_TOOLCHANGER() // for now not ifdefing per-extruder as well for simplicity
    StoreItem<DockPosition, defaults::dock_position, ItemFlag::calibrations, journal::hash("Dock Position 0")> dock_position_0;
    StoreItem<DockPosition, defaults::dock_position, ItemFlag::calibrations, journal::hash("Dock Position 1")> dock_position_1;
    StoreItem<DockPosition, defaults::dock_position, ItemFlag::calibrations, journal::hash("Dock Position 2")> dock_position_2;
    StoreItem<DockPosition, defaults::dock_position, ItemFlag::calibrations, journal::hash("Dock Position 3")> dock_position_3;
    StoreItem<DockPosition, defaults::dock_position, ItemFlag::calibrations, journal::hash("Dock Position 4")> dock_position_4;
    StoreItem<DockPosition, defaults::dock_position, ItemFlag::calibrations, journal::hash("Dock Position 5")> dock_position_5;

    DockPosition get_dock_position(uint8_t index);
    void set_dock_position(uint8_t index, DockPosition value);

    StoreItem<ToolOffset, defaults::tool_offset, ItemFlag::calibrations, journal::hash("Tool Offset 0")> tool_offset_0;
    StoreItem<ToolOffset, defaults::tool_offset, ItemFlag::calibrations, journal::hash("Tool Offset 1")> tool_offset_1;
    StoreItem<ToolOffset, defaults::tool_offset, ItemFlag::calibrations, journal::hash("Tool Offset 2")> tool_offset_2;
    StoreItem<ToolOffset, defaults::tool_offset, ItemFlag::calibrations, journal::hash("Tool Offset 3")> tool_offset_3;
    StoreItem<ToolOffset, defaults::tool_offset, ItemFlag::calibrations, journal::hash("Tool Offset 4")> tool_offset_4;
    StoreItem<ToolOffset, defaults::tool_offset, ItemFlag::calibrations, journal::hash("Tool Offset 5")> tool_offset_5;

    ToolOffset get_tool_offset(uint8_t index);
    void set_tool_offset(uint8_t index, ToolOffset value);
#endif

    StoreItemArray<EncodedFilamentType, EncodedFilamentType {}, ItemFlag::printer_state, journal::hash("Loaded Filament"), 8, EXTRUDERS> loaded_filament_type;

    /// User-defined filament ordering. Does not need to contain all the filaments - the rest will be appended to the back using the standard rules
    StoreItem<std::array<FilamentType, max_total_filament_count>, FilamentType::none, ItemFlag::user_presets, journal::hash("Filament Order")> filament_order;

    StoreItem<std::bitset<max_preset_filament_type_count>, defaults::visible_preset_filament_types, ItemFlag::user_presets, journal::hash("Visible Preset Filament Types")> visible_preset_filament_types;

    // We cannot use the constant in StoreItemArray, because it is scanned by a script and it would not be able to parse it
    static_assert(max_user_filament_type_count == 32);
    StoreItemArray<FilamentTypeParameters_EEPROM1, defaults::user_filament_parameters, ItemFlag::user_presets, journal::hash("User Filament Parameters"), 32, user_filament_type_count> user_filament_parameters;
#if HAS_CHAMBER_API()
    StoreItemArray<FilamentTypeParameters_EEPROM2, FilamentTypeParameters_EEPROM2 {}, ItemFlag::user_presets, journal::hash("User Filament Parameters 2"), 32, user_filament_type_count> user_filament_parameters_2;
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
    StoreItemArray<FilamentTypeParameters_EEPROM3, FilamentTypeParameters_EEPROM3 {}, ItemFlag::user_presets, journal::hash("User Filament Parameters 3"), 32, user_filament_type_count> user_filament_parameters_3;
#endif

    StoreItemArray<FilamentTypeParameters_EEPROM1, defaults::adhoc_filament_parameters, ItemFlag::user_presets, journal::hash("Adhoc Filament Parameters"), 8, adhoc_filament_type_count> adhoc_filament_parameters;
#if HAS_CHAMBER_API()
    StoreItemArray<FilamentTypeParameters_EEPROM2, FilamentTypeParameters_EEPROM2 {}, ItemFlag::user_presets, journal::hash("Adhoc Filament Parameters 2"), 8, adhoc_filament_type_count> adhoc_filament_parameters_2;
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
    StoreItemArray<FilamentTypeParameters_EEPROM3, FilamentTypeParameters_EEPROM3 {}, ItemFlag::user_presets, journal::hash("Adhoc Filament Parameters 3"), 8, adhoc_filament_type_count> adhoc_filament_parameters_3;
#endif

    StoreItem<std::bitset<max_user_filament_type_count>, defaults::visible_user_filament_types, ItemFlag::user_presets, journal::hash("Visible User Filament Types")> visible_user_filament_types;

    FilamentType get_filament_type(uint8_t index);
    void set_filament_type(uint8_t index, FilamentType value);

    // Note: hash is kept for backwards compatibility
    StoreItem<bool, false, ItemFlag::features, journal::hash("Heatup Bed")> filament_change_preheat_all;

    StoreItem<float, defaults::nozzle_diameter, ItemFlag::hw_config, journal::hash("Nozzle Diameter 0")> nozzle_diameter_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<float, defaults::nozzle_diameter, ItemFlag::hw_config, journal::hash("Nozzle Diameter 1")> nozzle_diameter_1;
    StoreItem<float, defaults::nozzle_diameter, ItemFlag::hw_config, journal::hash("Nozzle Diameter 2")> nozzle_diameter_2;
    StoreItem<float, defaults::nozzle_diameter, ItemFlag::hw_config, journal::hash("Nozzle Diameter 3")> nozzle_diameter_3;
    StoreItem<float, defaults::nozzle_diameter, ItemFlag::hw_config, journal::hash("Nozzle Diameter 4")> nozzle_diameter_4;
    StoreItem<float, defaults::nozzle_diameter, ItemFlag::hw_config, journal::hash("Nozzle Diameter 5")> nozzle_diameter_5;
#endif

    float get_nozzle_diameter(uint8_t index);
    void set_nozzle_diameter(uint8_t index, float value);

    // If this assert fails, we need to do some migrations on the following items
    static_assert(HOTENDS <= 8);

    /// Stores whether a nozzle is hardened (resistant to abrasive filament) or not. One bit per each hotend
    StoreItem<std::bitset<8>, 0, ItemFlag::hw_config, journal::hash("Nozzle is Hardened")> nozzle_is_hardened;

    /// Stores whether a nozzle is high-flow (supports high-flow print profile) or not. One bit per each hotend
    StoreItem<std::bitset<8>, defaults::nozzle_is_high_flow, ItemFlag::hw_config, journal::hash("Nozzle is High-Flow")> nozzle_is_high_flow;

    StoreItem<float, 0.0f, ItemFlag::calibrations, journal::hash("Homing Bump Divisor X")> homing_bump_divisor_x;
    StoreItem<float, 0.0f, ItemFlag::calibrations, journal::hash("Homing Bump Divisor Y")> homing_bump_divisor_y;

#if HAS_SIDE_LEDS()
    /// 0-255; 0 = disabled.
    StoreItem<uint8_t, 255, ItemFlag::user_interface, journal::hash("XBuddy Extension Chamber LEDs PWM")> side_leds_max_brightness;
    StoreItem<uint8_t, PWM255::from_percent(40).value, ItemFlag::user_interface, journal::hash("XBuddy Extension Chamber LEDs dimmed PWM")> side_leds_dimmed_brightness;
    /// Whether the side leds should dim down a bit when user is not interacting with the printer or stay on full power the whole time
    StoreItem<leds::DimmingEnabled, leds::DimmingEnabled::not_printing, ItemFlag::user_interface, journal::hash("Enable Side LEDs dimming")> side_leds_dimming_enabled;
#endif

    StoreItem<bool, true, ItemFlag::user_interface, journal::hash("Enable Serial Printing Screen")> serial_print_screen_enabled;

    StoreItem<bool, true, ItemFlag::user_interface, journal::hash("Enable Tool LEDs")> tool_leds_enabled;

    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer X")> odometer_x;
    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Y")> odometer_y;
    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Z")> odometer_z;

    float get_odometer_axis(uint8_t index);
    void set_odometer_axis(uint8_t index, float value);

    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Extruded Length 0")> odometer_extruded_length_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Extruded Length 1")> odometer_extruded_length_1;
    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Extruded Length 2")> odometer_extruded_length_2;
    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Extruded Length 3")> odometer_extruded_length_3;
    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Extruded Length 4")> odometer_extruded_length_4;
    StoreItem<float, 0.0f, ItemFlag::stats, journal::hash("Odometer Extruded Length 5")> odometer_extruded_length_5;
#endif

    float get_odometer_extruded_length(uint8_t index);
    void set_odometer_extruded_length(uint8_t index, float value);

    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Odometer Toolpicks 0")> odometer_toolpicks_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Odometer Toolpicks 1")> odometer_toolpicks_1;
    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Odometer Toolpicks 2")> odometer_toolpicks_2;
    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Odometer Toolpicks 3")> odometer_toolpicks_3;
    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Odometer Toolpicks 4")> odometer_toolpicks_4;
    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Odometer Toolpicks 5")> odometer_toolpicks_5;
#endif

    uint32_t get_odometer_toolpicks(uint8_t index);
    void set_odometer_toolpicks(uint8_t index, uint32_t value);

    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("MMU toolchanges")> mmu_changes;
    // Last time (in the mmu_changes) the user did maintenance
    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Last MMU maintenance")> mmu_last_maintenance;
    // A "leaky bucket" for MMU failures.
    StoreItem<uint16_t, 0, ItemFlag::stats, journal::hash("MMU fail bucket")> mmu_fail_bucket;

    StoreItem<HWCheckSeverity, defaults::hw_check_severity, ItemFlag::features, journal::hash("HW Check Nozzle")> hw_check_nozzle;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, ItemFlag::features, journal::hash("HW Check Model")> hw_check_model;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, ItemFlag::features, journal::hash("HW Check Firmware")> hw_check_firmware;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, ItemFlag::features, journal::hash("HW Check G-code")> hw_check_gcode_level;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, ItemFlag::features, journal::hash("HW Check Input Shaper")> hw_check_input_shaper;
#if HAS_GCODE_COMPATIBILITY()
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, ItemFlag::features, journal::hash("HW Check Compatibility")> hw_check_gcode_compatibility;
#endif
    template <typename F>
    auto visit_hw_check(HWCheckType type, const F &visitor) {
        switch (type) {

        case HWCheckType::nozzle:
            return visitor(hw_check_nozzle);

        case HWCheckType::model:
            return visitor(hw_check_model);

        case HWCheckType::firmware:
            return visitor(hw_check_firmware);

        case HWCheckType::gcode_level:
            return visitor(hw_check_gcode_level);

        case HWCheckType::input_shaper:
            return visitor(hw_check_input_shaper);
#if HAS_GCODE_COMPATIBILITY()
        case HWCheckType::gcode_compatibility:
            return visitor(hw_check_gcode_compatibility);
#endif
        }
    }

#if HAS_SELFTEST()
    StoreItem<SelftestResult, defaults::selftest_result, ItemFlag::calibrations, journal::hash("Selftest Result Gears")> selftest_result;
#endif

#if HAS_PHASE_STEPPING()
    StoreItem<TestResult, defaults::test_result_unknown, ItemFlag::calibrations, journal::hash("Test Result Phase Stepping")> selftest_result_phase_stepping;
#endif

    SelftestTool get_selftest_result_tool(uint8_t index);
    void set_selftest_result_tool(uint8_t index, SelftestTool value);

#if HAS_SHEET_PROFILES()
    StoreItem<uint8_t, 0, ItemFlag::printer_state, journal::hash("Active Sheet")> active_sheet;
    StoreItem<Sheet, defaults::sheet_0, ItemFlag::user_presets, journal::hash("Sheet 0")> sheet_0;
    StoreItem<Sheet, defaults::sheet_1, ItemFlag::user_presets, journal::hash("Sheet 1")> sheet_1;
    StoreItem<Sheet, defaults::sheet_2, ItemFlag::user_presets, journal::hash("Sheet 2")> sheet_2;
    StoreItem<Sheet, defaults::sheet_3, ItemFlag::user_presets, journal::hash("Sheet 3")> sheet_3;
    StoreItem<Sheet, defaults::sheet_4, ItemFlag::user_presets, journal::hash("Sheet 4")> sheet_4;
    StoreItem<Sheet, defaults::sheet_5, ItemFlag::user_presets, journal::hash("Sheet 5")> sheet_5;
    StoreItem<Sheet, defaults::sheet_6, ItemFlag::user_presets, journal::hash("Sheet 6")> sheet_6;
    StoreItem<Sheet, defaults::sheet_7, ItemFlag::user_presets, journal::hash("Sheet 7")> sheet_7;

    Sheet get_sheet(uint8_t index);
    void set_sheet(uint8_t index, Sheet value);
#endif

    // axis microsteps and rms current have a capital axis + '_' at the end in name because of trinamic.cpp. Can be removed once the macro there is removed
    StoreItem<float, defaults::axis_steps_per_unit_x, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Steps Per Unit X")> axis_steps_per_unit_x;
    StoreItem<float, defaults::axis_steps_per_unit_y, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Steps Per Unit Y")> axis_steps_per_unit_y;
    StoreItem<float, defaults::axis_steps_per_unit_z, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Steps Per Unit Z")> axis_steps_per_unit_z;
    StoreItem<float, defaults::axis_steps_per_unit_e0, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Steps Per Unit E0")> axis_steps_per_unit_e0;
    StoreItem<uint16_t, 0, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Microsteps X")> axis_microsteps_X_; // 0 - default value, !=0 - user value
    StoreItem<uint16_t, 0, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Microsteps Y")> axis_microsteps_Y_; // 0 - default value, !=0 - user value
    StoreItem<uint16_t, defaults::axis_microsteps_Z_, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Microsteps Z")> axis_microsteps_Z_;
    StoreItem<uint16_t, defaults::axis_microsteps_E0_, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Microsteps E0")> axis_microsteps_E0_;
    StoreItem<uint16_t, 0, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis RMS Current MA X")> axis_rms_current_ma_X_; // 0 - default value, !=0 - user value
    StoreItem<uint16_t, 0, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis RMS Current MA Y")> axis_rms_current_ma_Y_; // 0 - default value, !=0 - user value
    StoreItem<uint16_t, defaults::axis_rms_current_ma_Z_, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis RMS Current MA Z")> axis_rms_current_ma_Z_;
    StoreItem<uint16_t, defaults::axis_rms_current_ma_E0_, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis RMS Current MA E0")> axis_rms_current_ma_E0_;
    StoreItem<float, defaults::axis_z_max_pos_mm, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Axis Z Max Pos MM")> axis_z_max_pos_mm;

#if HAS_HOTEND_TYPE_SUPPORT()
    // Nozzle Sock has is here for backwards compatibility (should be binary compatible)
    StoreItemArray<HotendType, defaults::hotend_type, ItemFlag::hw_config, journal::hash("Hotend Type Per Tool"), 8, HOTENDS> hotend_type;
#endif

    StoreItem<int16_t, defaults::homing_sens_x, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("Homing Sens X")> homing_sens_x; // X axis homing sensitivity
    StoreItem<int16_t, defaults::homing_sens_y, ItemFlag::calibrations | ItemFlag::common_misconfigurations, journal::hash("Homing Sens Y")> homing_sens_y; // Y axis homing sensitivity

    StoreItem<bool, true, ItemFlag::features, journal::hash("Stuck filament detection V2")> stuck_filament_detection;

    StoreItem<bool, false, ItemFlag::features, journal::hash("Stealth mode")> stealth_mode;

    StoreItem<bool, true, ItemFlag::features, journal::hash("Input Shaper Axis X Enabled")> input_shaper_axis_x_enabled;
    StoreItem<input_shaper::AxisConfig, input_shaper::axis_x_default, ItemFlag::calibrations, journal::hash("Input Shaper Axis X Config")> input_shaper_axis_x_config;
    StoreItem<bool, true, ItemFlag::features, journal::hash("Input Shaper Axis Y Enabled")> input_shaper_axis_y_enabled;
    StoreItem<input_shaper::AxisConfig, input_shaper::axis_y_default, ItemFlag::calibrations, journal::hash("Input Shaper Axis Y Config")> input_shaper_axis_y_config;
    StoreItem<bool, input_shaper::weight_adjust_enabled_default, ItemFlag::calibrations, journal::hash("Input Shaper Weight Adjust Y Enabled V2")> input_shaper_weight_adjust_y_enabled;
    StoreItem<input_shaper::WeightAdjustConfig, input_shaper::weight_adjust_y_default, ItemFlag::calibrations, journal::hash("Input Shaper Weight Adjust Y Config")> input_shaper_weight_adjust_y_config;

    input_shaper::Config get_input_shaper_config();
    void set_input_shaper_config(const input_shaper::Config &);

    input_shaper::AxisConfig get_input_shaper_axis_config(AxisEnum axis);
    void set_input_shaper_axis_config(AxisEnum axis, const input_shaper::AxisConfig &);

    /// If set to true, will run the set HW defaults section in perform_config_check (and set itself to false) on next boot
    StoreItem<bool, false, ItemFlag::special, journal::hash("Force Default HW Config")> force_default_hw_config;

    /// FW base printer model from the last boot of the printer.
    /// Used for detecting when the printer has been upgraded to a different base model with the same board (for example MK3.5 -> MK3.9)
    /// We want to detect those cases and force a factory reset, because some config store might not be compatible between different firmwares.
    StoreItem<PrinterModel, static_cast<PrinterModel>(-1), ItemFlag::hw_config, journal::hash("Last Boot Base Printer Model")> last_boot_base_printer_model;

#if PRINTER_IS_PRUSA_MK3_5()
    StoreItem<bool, false, ItemFlag::hw_config, journal::hash("Has Alt Fans")> has_alt_fans;
#endif

#if HAS_PHASE_STEPPING()
    static constexpr bool phase_stepping_ram_only = !HAS_PHASE_STEPPING_TOGGLE();
    StoreItem<bool, defaults::phase_stepping_enabled_x, ItemFlag::features, journal::hash("Phase Stepping Enabled X"), 1, phase_stepping_ram_only> phase_stepping_enabled_x;
    StoreItem<bool, defaults::phase_stepping_enabled_y, ItemFlag::features, journal::hash("Phase Stepping Enabled Y"), 1, phase_stepping_ram_only> phase_stepping_enabled_y;

    bool get_phase_stepping_enabled();
    bool get_phase_stepping_enabled(AxisEnum axis);
    void set_phase_stepping_enabled(AxisEnum axis, bool new_state);
#endif

#if XL_ENCLOSURE_SUPPORT()
    StoreItem<bool, false, ItemFlag::features, journal::hash("XL Enclosure Enabled")> xl_enclosure_enabled;
    StoreItem<TestResult, defaults::test_result_unknown, ItemFlag::calibrations, journal::hash("XL Enclosure Fan Selftest Result")> xl_enclosure_fan_selftest_result;
#endif

#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
    StoreItem<int8_t, 0, ItemFlag::calibrations, journal::hash("Left Bed Correction")> left_bed_correction;
    StoreItem<int8_t, 0, ItemFlag::calibrations, journal::hash("Right Bed Correction")> right_bed_correction;
    StoreItem<int8_t, 0, ItemFlag::calibrations, journal::hash("Front Bed Correction")> front_bed_correction;
    StoreItem<int8_t, 0, ItemFlag::calibrations, journal::hash("Rear Bed Correction")> rear_bed_correction;
#endif

#if HAS_EXTENDED_PRINTER_TYPE()
    StoreItem<uint8_t, 0, ItemFlag::hw_config, journal::hash("Extended Printer Type")> extended_printer_type;
#endif

#if HAS_INPUT_SHAPER_CALIBRATION()
    StoreItem<TestResult, TestResult_Unknown, ItemFlag::calibrations, journal::hash("Input Shaper Calibration")> selftest_result_input_shaper_calibration;
#endif

#if HAS_I2C_EXPANDER()
    StoreItem<uint8_t, 0, ItemFlag::printer_state, journal::hash("IO Expander's Configuration Register")> io_expander_config_register;
    StoreItem<uint8_t, 0, ItemFlag::printer_state, journal::hash("IO Expander's Output Register")> io_expander_output_register;
    StoreItem<uint8_t, 0, ItemFlag::printer_state, journal::hash("IO Expander's Polarity Register")> io_expander_polarity_register;
#endif // HAS_I2C_EXPANDER()

#if HAS_XBUDDY_EXTENSION()
    StoreItem<XBEFanTestResults, XBEFanTestResults {}, ItemFlag::calibrations, journal::hash("XBE Chamber fan selftest results")> xbe_fan_test_results;
    // Has flag of hw_config because the user toggles this as part of hw_config in printer setup
    StoreItem<bool, true, ItemFlag::hw_config, journal::hash("XBE USB Host power")> xbe_usb_power;
    StoreItem<uint8_t, 102, ItemFlag::features, journal::hash("XBuddy Extension Chamber Fan Max Control Limit")> xbe_cooling_fan_max_auto_pwm;
    StoreItem<uint8_t, PWM255::from_percent(70).value, ItemFlag::features, journal::hash("XBE Filtration Fan Max Auto PWM")> xbe_filtration_fan_max_auto_pwm;
#endif

#if HAS_DOOR_SENSOR_CALIBRATION()
    StoreItem<TestResult, defaults::test_result_unknown, ItemFlag::calibrations, journal::hash("Selftest Result - Door Sensor")> selftest_result_door_sensor;
#endif

#if HAS_EMERGENCY_STOP()
    StoreItem<bool, false, ItemFlag::features, journal::hash("Emergency stop enable v2")> emergency_stop_enable;
#endif

    StoreItem<bool, false, ItemFlag::features, journal::hash("Happy Printing Seen")> happy_printing_seen;

#if HAS_ILI9488_DISPLAY()
    StoreItem<bool, false, ItemFlag::hw_config | ItemFlag::common_misconfigurations, journal::hash("Reduce Display Baudrate")> reduce_display_baudrate;
#endif

#if HAS_PRECISE_HOMING_COREXY()
    StoreItem<CoreXYGridOrigin, COREXY_NO_GRID_ORIGIN, ItemFlag::calibrations, journal::hash("CoreXY calibrated grid origin")> corexy_grid_origin;

    /// Whether to automatically calibrate precise homing when deemed necessary
    /// Tristate::other = ask the user
    StoreItem<Tristate, Tristate::other, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("Auto-recalibrate precise homing")> auto_recalibrate_precise_homing;

    /// History whether a homing point was stable after precise homing. High number of unstable homings will result in calibration prompt.
    /// Implemented as a rotating bit buffer (pushed after each successful refinement); ones represent unstable refinements
    StoreItem<uint16_t, 0, ItemFlag::printer_state, journal::hash("Precise Homing Instability History")> precise_homing_instability_history;
#endif
#if HAS_PRECISE_HOMING_COREXY() && HAS_TRINAMIC && defined(XY_HOMING_MEASURE_SENS_MIN)
    StoreItem<CoreXYHomeTMCSens, COREXY_NO_HOME_TMC_SENS, ItemFlag::calibrations, journal::hash("CoreXY home TMC calibration")> corexy_home_tmc_sens;
#endif
#if HAS_PRECISE_HOMING()
    static constexpr uint8_t precise_homing_axis_sample_count = 9;
    static constexpr uint8_t precise_homing_axis_count = 2;

    /// Per-axis circular buffer that keeps \p precise_homing_axis_sample_count latest hoing samples
    StoreItemArray<uint16_t, uint16_t { 0xffff }, ItemFlag::calibrations, journal::hash("Precise homing samples"), 32, precise_homing_axis_count * precise_homing_axis_sample_count> precise_homing_sample_history;
    StoreItemArray<uint8_t, uint8_t { 0 }, ItemFlag::calibrations, journal::hash("Precise homing samples index"), 3, precise_homing_axis_count> precise_homing_sample_history_index;
#endif

#if HAS_CHAMBER_FILTRATION_API()
    StoreItem<buddy::ChamberFiltrationBackend, buddy::ChamberFiltrationBackend::none, ItemFlag::hw_config, journal::hash("Chamber filtration backend")> chamber_filtration_backend;
    StoreItem<bool, true, ItemFlag::features, journal::hash("Chamber filtration post print enable")> chamber_post_print_filtration_enable;
    StoreItem<bool, true, ItemFlag::features, journal::hash("Chamber filtration print enable")> chamber_print_filtration_enable;
    StoreItem<uint8_t, 10, ItemFlag::features, journal::hash("Chamber filtration post print duration")> chamber_post_print_filtration_duration_min;
    StoreItem<PWM255, PWM255::from_percent(40).value, ItemFlag::features, journal::hash("Chamber mid print filtration pwm")> chamber_mid_print_filtration_pwm;
    StoreItem<PWM255, PWM255::from_percent(40).value, ItemFlag::features, journal::hash("Chamber post print filtration pwm")> chamber_post_print_filtration_pwm;
    StoreItem<bool, false, ItemFlag::features, journal::hash("Chamber filtration always on")> chamber_filtration_always_on;

    /// How long the filter has been used for (= fan is blowing through the filter), in seconds. Resets on filter change.
    StoreItem<uint32_t, 0, ItemFlag::stats, journal::hash("Chamber filter time used ")> chamber_filter_time_used_s;

    StoreItem<bool, false, ItemFlag::stats, journal::hash("Chamber filter early expiration warning shown")> chamber_filter_early_expiration_warning_shown;

    /// If set, shown next chamber warning only after the specified timestamp.
    /// The unix timestamp has been divided by 1024 to fit into int32 even after year 2038
    StoreItem<int32_t, 0, ItemFlag::stats, journal::hash("Chamber filter expiration postpone timestamp")> chamber_filter_expiration_postpone_timestamp_1024;
#endif

#if HAS_PRINT_FAN_TYPE()
    StoreItemArray<PrintFanType, PrintFanType::default_value, ItemFlag::hw_config, journal::hash("Print Fan Type Per Tool"), 8, HOTENDS> print_fan_type;
#endif

#if HAS_AUTO_RETRACT()
    StoreItem<bool, true, ItemFlag::printer_state, journal::hash("Pre-nozzle cleaning retraction enabled")> pre_nozzle_cleaning_retraction_enable;

    /// Global enable for auto-retract
    /// Setting FALSE does NOT disable the feature completely, just prevents MAYBE_DERETRACT() from happening
    /// Retracted filaments will auto-deretract in every case
    StoreItem<bool, true, ItemFlag::features | ItemFlag::common_misconfigurations, journal::hash("Enable auto-retract")> auto_retract_enabled;

    // Each hotend holds retracted distance. This value is compressed (casted to uint8) to range < 0 ; 255 > with 255 being special value reserved for unknown distance
    // DO NOT ACCESS THIS ARRAY DIRECTLY, user getter/setter instead
    StoreItemArray<uint8_t, uint8_t { 255 }, ItemFlag::printer_state, journal::hash("Filament retracted distances"), 8, HOTENDS> filament_retracted_distances;

    // Casts float of range < 0.0f ; 254f > to uint8. Value 255 is reserved to unknown value
    void set_filament_retracted_distance(uint8_t tool_idx, std::optional<float> dist);
    std::optional<float> get_filament_retracted_distance(uint8_t tool_idx);

    static_assert(HOTENDS <= 8);
#endif

#if HAS_CHAMBER_VENTS()
    StoreItem<bool, true, ItemFlag::features, journal::hash("Check chamber ventilation state")> check_chamber_vent_state;
    StoreItem<bool, true, ItemFlag::hw_config, journal::hash("Auto chamber vent enabled")> auto_chamber_vent_enabled;

    VentControl get_vent_control();
    void set_vent_control(VentControl state);
#endif

#if HAS_MANUAL_BELT_TUNING()
    StoreItem<bool, false, ItemFlag::calibrations, journal::hash("Manual Belt Tuning Completed")> manual_belt_tuning_completed;
#endif

private:
    void perform_config_migrations();
};

/**
 * @brief Holds all deprecated store items. To deprecate an item, move it from CurrentStore to this DeprecatedStore. If you're adding a newer version of an item, make sure the succeeding CurentStore::StoreItem has a different HASHED ID than the one deprecated (ie successor to hash("Sound Mode") could be hash("Sound Mode V2"))
 *
 * This is pseudo 'graveyard' of old store items, so that it can be verified IDs don't cause conflicts and old 'default' values can be fetched if needed.
 *
 * If you want to migrate existing data to 'newer version', add a migration_function with the ids as well (see below). If all you want is to delete an item, just moving it here from CurrentStore is enough.
 *
 * !!! MAKE SURE to move StoreItems from CurrentStore to here KEEP their HASHED ID !!! (to make sure backend works correctly when scanning through entries)
 */
struct DeprecatedStore
#if HAS_CONFIG_STORE_WO_BACKEND()
    : public no_backend::NBJournalDeprecatedStoreConfig
#else
    : public journal::DeprecatedStoreConfig<journal::Backend>
#endif
{
    // There was a ConfigStore version already before last eeprom version of SelftestResult was made, so it doesn't have old eeprom predecessor
    StoreItem<SelftestResult_pre_23, defaults::selftest_result_pre_23, journal::hash("Selftest Result")> selftest_result_pre_23;
    // Selftest Result version before adding Gearbox Alignment result to EEPROM
    StoreItem<SelftestResult_pre_gears, defaults::selftest_result_pre_gears, journal::hash("Selftest Result V23")> selftest_result_pre_gears;

    // Changing Filament Sensor default state to remove necessity of FS dialog on startup
    StoreItem<bool, true, journal::hash("FSensor Enabled")> fsensor_enabled_v1;

    // An item was added to the middle of the footer enum and it caused eeprom corruption. This store footer item  was deleted and a new one is created without migration so as to force default footer value onto everyone, which is better than 'random values' (especially on mini where it could cause duplicated items shown). Default value was removed since we no longer need to keep it
    StoreItem<uint32_t, 0, journal::hash("Footer Setting")> footer_setting_v1;

    StoreItem<footer::Item, defaults::footer_setting_0, journal::hash("Footer Setting 0")> footer_setting_0_v2;
#if FOOTER_ITEMS_PER_LINE__ > 1
    StoreItem<footer::Item, defaults::footer_setting_1, journal::hash("Footer Setting 1")> footer_setting_1_v2;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    StoreItem<footer::Item, defaults::footer_setting_2, journal::hash("Footer Setting 2")> footer_setting_2_v2;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    StoreItem<footer::Item, defaults::footer_setting_3, journal::hash("Footer Setting 3")> footer_setting_3_v2;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    StoreItem<footer::Item, defaults::footer_setting_4, journal::hash("Footer Setting 4")> footer_setting_4_v2;
#endif

    // Filament types loaded in extruders
    StoreItem<EncodedFilamentType, EncodedFilamentType {}, journal::hash("Filament Type 0")> filament_type_0;
#if EXTRUDERS > 1 // for now only doing one ifdef for simplicity
    StoreItem<EncodedFilamentType, EncodedFilamentType {}, journal::hash("Filament Type 1")> filament_type_1;
    StoreItem<EncodedFilamentType, EncodedFilamentType {}, journal::hash("Filament Type 2")> filament_type_2;
    StoreItem<EncodedFilamentType, EncodedFilamentType {}, journal::hash("Filament Type 3")> filament_type_3;
    StoreItem<EncodedFilamentType, EncodedFilamentType {}, journal::hash("Filament Type 4")> filament_type_4;
    StoreItem<EncodedFilamentType, EncodedFilamentType {}, journal::hash("Filament Type 5")> filament_type_5;
#endif

    // There was wrong default value for XL, so V2 version was introduced to reset it to proper default value
    StoreItem<bool, true, journal::hash("Input Shaper Weight Adjust Y Enabled")> input_shaper_weight_adjust_y_enabled;

    StoreItem<bool, false, journal::hash("Stuck filament detection")> stuck_filament_detection;

    /// Changed into ExtendedPrinterType
    /// This was used everywhere as determining if the printer is MK3.9 (== false) :/
    // All other printers seem to have it true
    StoreItem<bool, true, journal::hash("400 step motors on X and Y axis")> xy_motors_400_step;

    // Unified WIFI and LAN hostnames - BFW-5523
    StoreItem<std::array<char, lan_hostname_max_len + 1>, defaults::net_hostname, journal::hash("LAN Hostname")> lan_hostname;
    StoreItem<std::array<char, lan_hostname_max_len + 1>, defaults::net_hostname, journal::hash("WIFI Hostname")> wifi_hostname;

#if PRINTER_IS_PRUSA_XL()
    StoreItem<TestResult, defaults::test_result_unknown, journal::hash("Selftest Result - Nozzle Diameter")> selftest_result_nozzle_diameter;
#endif

    StoreItem<uint8_t, 0, journal::hash("Metrics Allow")> metrics_allow; ///< Metrics are allowed to be enabled

    StoreItem<bool, true, journal::hash("Run XYZ Calibration")> run_xyz_calib;
    StoreItem<bool, true, journal::hash("Run First Layer")> run_first_layer;

    StoreItem<uint8_t, 0, journal::hash("Nozzle Type")> nozzle_type;

    StoreItem<bool, true, journal::hash("Enable Side LEDs")> side_leds_enabled;

    StoreItem<float, 0, journal::hash("Loadcell Scale")> loadcell_scale;
    StoreItem<float, 0, journal::hash("Loadcell Threshold Static")> loadcell_threshold_static;
    StoreItem<float, 0, journal::hash("Loadcell Hysteresis")> loadcell_hysteresis;
    StoreItem<float, 0, journal::hash("Loadcell Threshold Continuous")> loadcell_threshold_continuous;

    StoreItem<HWCheckSeverity, defaults::hw_check_severity, journal::hash("HW Check Fan Compatibility")> hw_check_fan_compatibility;

#if HAS_HOTEND_TYPE_SUPPORT()
    StoreItem<HotendType, defaults::hotend_type, journal::hash("Nozzle Sock")> hotend_type_single_hotend;
#endif

    StoreItem<bool, false, journal::hash("USB MSC Enabled")> usb_msc_enabled;

    struct RestoreZPosition {
        float current_position_z;
        uint8_t axis_known_position;
        constexpr auto operator<=>(const RestoreZPosition &) const = default;
    };
    static inline constexpr RestoreZPosition restore_z_default_position { NAN, 0 };
    StoreItem<RestoreZPosition, restore_z_default_position, journal::hash("Restore Z Coordinate After Boot")> restore_z_after_boot;

#if XL_ENCLOSURE_SUPPORT()
    StoreItem<uint8_t, 6, journal::hash("XL Enclosure Flags")> xl_enclosure_flags;
    StoreItem<int64_t, defaults::int64_zero, journal::hash("XL Enclosure Filter Timer")> xl_enclosure_filter_timer;
    StoreItem<uint8_t, defaults::uint8_percentage_80, journal::hash("XL Enclosure Fan Manual Setting")> xl_enclosure_fan_manual;
    StoreItem<uint8_t, 10, journal::hash("XL Enclosure Post Print Duration")> xl_enclosure_post_print_duration;
#endif

#if HAS_EMERGENCY_STOP()
    StoreItem<bool, true, journal::hash("Emergency stop enable")> emergency_stop_enable;
#endif

#if HAS_SIDE_LEDS()
    StoreItem<leds::DimmingEnabled, leds::DimmingEnabled::not_printing, journal::hash("Side LEDs dimming with camera")> side_leds_dimming_enabled_with_camera;
    #if HAS_XBUDDY_EXTENSION()
    StoreItem<uint8_t, 255, journal::hash("Chamber LEDs PWM with Camera")> side_leds_max_brightness_with_camera;
    #endif
#endif

#if HAS_AUTO_RETRACT()
    StoreItem<uint8_t, 0, journal::hash("Filament auto-retracted")> filament_auto_retracted_bitset;
#endif

    /*
        Having these guys in the comments is actually engouh for the scraper to find the journal hash

        StoreItem<uint32_t, defaults::extruder_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Extruder FS Value Span 0")> extruder_fs_value_span_0;
        StoreItem<uint32_t, defaults::extruder_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Extruder FS Value Span 1")> extruder_fs_value_span_1;
        StoreItem<uint32_t, defaults::extruder_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Extruder FS Value Span 2")> extruder_fs_value_span_2;
        StoreItem<uint32_t, defaults::extruder_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Extruder FS Value Span 3")> extruder_fs_value_span_3;
        StoreItem<uint32_t, defaults::extruder_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Extruder FS Value Span 4")> extruder_fs_value_span_4;
        StoreItem<uint32_t, defaults::extruder_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Extruder FS Value Span 5")> extruder_fs_value_span_5;
        StoreItem<uint32_t, defaults::side_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Side FS Value Span 0")> side_fs_value_span_0;
        StoreItem<uint32_t, defaults::side_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Side FS Value Span 1")> side_fs_value_span_1;
        StoreItem<uint32_t, defaults::side_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Side FS Value Span 2")> side_fs_value_span_2;
        StoreItem<uint32_t, defaults::side_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Side FS Value Span 3")> side_fs_value_span_3;
        StoreItem<uint32_t, defaults::side_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Side FS Value Span 4")> side_fs_value_span_4;
        StoreItem<uint32_t, defaults::side_fs_value_span, ItemFlag::calibrations | ItemFlag::dev_items, journal::hash("Side FS Value Span 5")> side_fs_value_span_5;
        */

    // This was replaced by 2 separate items for network and hw_config
    StoreItem<bool, false, journal::hash("Printer setup done")> printer_setup_done;
};

} // namespace config_store_ns
