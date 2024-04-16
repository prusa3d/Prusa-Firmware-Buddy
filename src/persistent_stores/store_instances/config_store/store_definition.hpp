#pragma once
#include <Marlin/src/inc/MarlinConfigPre.h>

#include <common/nozzle_type.hpp>
#include <common/hotend_type.hpp>
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
#include <filament.hpp>
#include <selftest_result.hpp>
#include <common/sheet.hpp>
#include <module/prusa/dock_position.hpp>
#include <module/prusa/tool_offset.hpp>
#include <filament_sensors_remap_data.hpp>
#include <feature/prusa/restore_z_storage.h>
#include <option/has_loadcell.h>
#include <option/has_side_fsensor.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#include <option/has_selftest.h>
#include <option/has_phase_stepping.h>

namespace config_store_ns {
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
    // wizard flags
    StoreItem<bool, defaults::bool_true, journal::hash("Run Selftest")> run_selftest;
    StoreItem<bool, defaults::bool_true, journal::hash("Run XYZ Calibration")> run_xyz_calib;
    StoreItem<bool, defaults::bool_true, journal::hash("Run First Layer")> run_first_layer;

    /// Global filament sensor enable
    StoreItem<bool, defaults::fsensor_enabled, journal::hash("FSensor Enabled V2")> fsensor_enabled;

    /// Bitfield of enabled side filament sensors
    StoreItem<uint8_t, defaults::uint8_t_ff, journal::hash("Extruder FSensors enabled")> fsensor_side_enabled_bits;

    /// Bitfield of enabled toolhead filament sensors
    StoreItem<uint8_t, defaults::uint8_t_ff, journal::hash("Side FSensors enabled")> fsensor_extruder_enabled_bits;

    // nozzle PID variables
    StoreItem<float, defaults::pid_nozzle_p, journal::hash("PID Nozzle P")> pid_nozzle_p;
    StoreItem<float, defaults::pid_nozzle_i, journal::hash("PID Nozzle I")> pid_nozzle_i;
    StoreItem<float, defaults::pid_nozzle_d, journal::hash("PID Nozzle D")> pid_nozzle_d;

    // bed PID variables
    StoreItem<float, defaults::pid_bed_p, journal::hash("PID Bed P")> pid_bed_p;
    StoreItem<float, defaults::pid_bed_i, journal::hash("PID Bed I")> pid_bed_i;
    StoreItem<float, defaults::pid_bed_d, journal::hash("PID Bed D")> pid_bed_d;

    // LAN settings
    // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1
    StoreItem<uint8_t, defaults::uint8_t_zero, journal::hash("LAN Flag")> lan_flag;
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("LAN IP4 Address")> lan_ip4_addr; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("LAN IP4 Mask")> lan_ip4_mask; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("LAN IP4 Gateway")> lan_ip4_gateway; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("LAN IP4 DNS1")> lan_ip4_dns1; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("LAN IP4 DNS2")> lan_ip4_dns2; // X.X.X.X address encoded
    StoreItem<std::array<char, lan_hostname_max_len + 1>, defaults::net_hostname, journal::hash("LAN Hostname")> lan_hostname;

    StoreItem<int8_t, defaults::lan_timezone, journal::hash("LAN Timezone")> timezone; // hour difference from UTC
    StoreItem<time_tools::TimezoneOffsetMinutes, defaults::timezone_minutes, journal::hash("Timezone Minutes")> timezone_minutes; // minutes offset for hour difference from UTC
    StoreItem<time_tools::TimezoneOffsetSummerTime, defaults::timezone_summer, journal::hash("Timezone Summertime")> timezone_summer; // Summertime hour offset

    // WIFI settings
    // wifi_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1, wifi_flag & 0b1100 -> reserved, previously ap_sec_t security
    StoreItem<uint8_t, defaults::uint8_t_zero, journal::hash("WIFI Flag")> wifi_flag;
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("WIFI IP4 Address")> wifi_ip4_addr; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("WIFI IP4 Mask")> wifi_ip4_mask; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("WIFI IP4 Gateway")> wifi_ip4_gateway; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("WIFI IP4 DNS1")> wifi_ip4_dns1; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("WIFI IP4 DNS2")> wifi_ip4_dns2; // X.X.X.X address encoded
    StoreItem<std::array<char, lan_hostname_max_len + 1>, defaults::net_hostname, journal::hash("WIFI Hostname")> wifi_hostname;
    StoreItem<std::array<char, wifi_max_ssid_len + 1>, defaults::wifi_ap_ssid, journal::hash("WIFI AP SSID")> wifi_ap_ssid;
    StoreItem<std::array<char, wifi_max_passwd_len + 1>, defaults::wifi_ap_password, journal::hash("WIFI AP Password")> wifi_ap_password;

    StoreItem<eSOUND_MODE, defaults::sound_mode, journal::hash("Sound Mode")> sound_mode;
    StoreItem<uint8_t, defaults::sound_volume, journal::hash("Sound Volume")> sound_volume;
    StoreItem<uint16_t, defaults::language, journal::hash("Language")> language;
    StoreItem<uint8_t, defaults::uint8_t_zero, journal::hash("File Sort")> file_sort; // filebrowser file sort options
    StoreItem<bool, defaults::bool_true, journal::hash("Menu Timeout")> menu_timeout; // on / off menu timeout flag
    StoreItem<bool, defaults::bool_true, journal::hash("Devhash in QR")> devhash_in_qr; // on / off sending UID in QR

    StoreItem<footer::Item, defaults::footer_setting_0, journal::hash("Footer Setting 0 v3")> footer_setting_0;
#if FOOTER_ITEMS_PER_LINE__ > 1
    StoreItem<footer::Item, defaults::footer_setting_1, journal::hash("Footer Setting 1 v3")> footer_setting_1;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    StoreItem<footer::Item, defaults::footer_setting_2, journal::hash("Footer Setting 2 v3")> footer_setting_2;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    StoreItem<footer::Item, defaults::footer_setting_3, journal::hash("Footer Setting 3 v3")> footer_setting_3;
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    StoreItem<footer::Item, defaults::footer_setting_4, journal::hash("Footer Setting 4 v3")> footer_setting_4;
#endif

    footer::Item get_footer_setting(uint8_t index);
    void set_footer_setting(uint8_t index, footer::Item value);

    StoreItem<uint32_t, defaults::footer_draw_type, journal::hash("Footer Draw Type")> footer_draw_type;
    StoreItem<bool, defaults::bool_true, journal::hash("Fan Check Enabled")> fan_check_enabled;
    StoreItem<bool, defaults::bool_true, journal::hash("FS Autoload Enabled")> fs_autoload_enabled;

    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Odometer Time")> odometer_time;
    StoreItem<uint8_t, defaults::uint8_t_zero, journal::hash("Active NetDev")> active_netdev; // active network device
    StoreItem<bool, defaults::bool_true, journal::hash("PrusaLink Enabled")> prusalink_enabled;
    StoreItem<std::array<char, pl_password_size>, defaults::prusalink_password, journal::hash("PrusaLink Password")> prusalink_password;

    StoreItem<bool, defaults::bool_false, journal::hash("USB MSC Enabled")> usb_msc_enabled;

    StoreItem<std::array<char, connect_host_size + 1>, defaults::connect_host, journal::hash("Connect Host")> connect_host;
    StoreItem<std::array<char, connect_token_size + 1>, defaults::connect_token, journal::hash("Connect Token")> connect_token;
    StoreItem<uint16_t, defaults::connect_port, journal::hash("Connect Port")> connect_port;
    StoreItem<bool, defaults::bool_true, journal::hash("Connect TLS")> connect_tls;
    StoreItem<bool, defaults::bool_false, journal::hash("Connect Enabled")> connect_enabled;

    // Metrics
    StoreItem<MetricsAllow, defaults::metrics_allow, journal::hash("Metrics Allow")> metrics_allow; ///< Metrics are allowed to be enabled
    StoreItem<std::array<char, metrics_host_size + 1>, defaults::metrics_host, journal::hash("Metrics Host")> metrics_host; ///< Host used to allow and init metrics
    StoreItem<uint16_t, defaults::metrics_port, journal::hash("Metrics Port")> metrics_port; ///< Port used to allow and init metrics
    StoreItem<uint16_t, defaults::syslog_port, journal::hash("Log Port")> syslog_port; ///< Port used to allow and init log (uses metrics_host)
    StoreItem<bool, defaults::metrics_init, journal::hash("Metrics Init")> metrics_init; ///< Init metrics host after start

    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Job ID")> job_id; // print job id incremented at every print start

    StoreItem<bool, defaults::crash_enabled, journal::hash("Crash Enabled")> crash_enabled;
    StoreItem<int16_t, defaults::crash_sens_x, journal::hash("Crash Sens X")> crash_sens_x; // X axis crash sensitivity
    StoreItem<int16_t, defaults::crash_sens_y, journal::hash("Crash Sens Y")> crash_sens_y; // Y axis crash sensitivity

    // X axis max crash period (speed) threshold
    StoreItem<uint16_t, defaults::crash_max_period_x, journal::hash("Crash Sens Max Period X")> crash_max_period_x;
    // Y axis max crash period (speed) threshold
    StoreItem<uint16_t, defaults::crash_max_period_y, journal::hash("Crash Sens Max Period Y")> crash_max_period_y;
    StoreItem<bool, defaults::crash_filter, journal::hash("Crash Filter")> crash_filter; // Stallguard filtration
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Crash Count X")> crash_count_x; // number of crashes of X axis in total
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Crash Count Y")> crash_count_y; // number of crashes of Y axis in total
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Power Panics Count")> power_panics_count; // number of power losses in total

    StoreItem<time_tools::TimeFormat, defaults::time_format, journal::hash("Time Format")> time_format;

#if HAS_LOADCELL()
    StoreItem<float, defaults::loadcell_scale, journal::hash("Loadcell Scale")> loadcell_scale;
    StoreItem<float, defaults::loadcell_threshold_static, journal::hash("Loadcell Threshold Static")> loadcell_threshold_static;
    StoreItem<float, defaults::loadcell_hysteresis, journal::hash("Loadcell Hysteresis")> loadcell_hysteresis;
    StoreItem<float, defaults::loadcell_threshold_continuous, journal::hash("Loadcell Threshold Continuous")> loadcell_threshold_continuous;
#endif

    // filament sensor values:
    // ref value: value of filament sensor in moment of calibration (w/o filament present)
    // value span: minimal difference of raw values between the two states of the filament sensor
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, journal::hash("Extruder FS Ref Value 0")> extruder_fs_ref_nins_value_0;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, journal::hash("Extruder FS INS Ref Value 0")> extruder_fs_ref_ins_value_0;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, journal::hash("Extruder FS Value Span 0")> extruder_fs_value_span_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, journal::hash("Extruder FS Ref Value 1")> extruder_fs_ref_nins_value_1;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, journal::hash("Extruder FS INS Ref Value 1")> extruder_fs_ref_ins_value_1;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, journal::hash("Extruder FS Value Span 1")> extruder_fs_value_span_1;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, journal::hash("Extruder FS Ref Value 2")> extruder_fs_ref_nins_value_2;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, journal::hash("Extruder FS INS Ref Value 2")> extruder_fs_ref_ins_value_2;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, journal::hash("Extruder FS Value Span 2")> extruder_fs_value_span_2;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, journal::hash("Extruder FS Ref Value 3")> extruder_fs_ref_nins_value_3;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, journal::hash("Extruder FS INS Ref Value 3")> extruder_fs_ref_ins_value_3;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, journal::hash("Extruder FS Value Span 3")> extruder_fs_value_span_3;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, journal::hash("Extruder FS Ref Value 4")> extruder_fs_ref_nins_value_4;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, journal::hash("Extruder FS INS Ref Value 4")> extruder_fs_ref_ins_value_4;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, journal::hash("Extruder FS Value Span 4")> extruder_fs_value_span_4;
    StoreItem<int32_t, defaults::extruder_fs_ref_nins_value, journal::hash("Extruder FS Ref Value 5")> extruder_fs_ref_nins_value_5;
    StoreItem<int32_t, defaults::extruder_fs_ref_ins_value, journal::hash("Extruder FS INS Ref Value 5")> extruder_fs_ref_ins_value_5;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, journal::hash("Extruder FS Value Span 5")> extruder_fs_value_span_5;
#endif

#if HAS_SIDE_FSENSOR() // for now not ifdefing per-extruder as well for simplicity
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, journal::hash("Side FS Ref Value 0")> side_fs_ref_nins_value_0;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, journal::hash("Side FS Ref INS Value 0")> side_fs_ref_ins_value_0;
    StoreItem<uint32_t, defaults::side_fs_value_span, journal::hash("Side FS Value Span 0")> side_fs_value_span_0;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, journal::hash("Side FS Ref Value 1")> side_fs_ref_nins_value_1;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, journal::hash("Side FS Ref INS Value 1")> side_fs_ref_ins_value_1;
    StoreItem<uint32_t, defaults::side_fs_value_span, journal::hash("Side FS Value Span 1")> side_fs_value_span_1;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, journal::hash("Side FS Ref Value 2")> side_fs_ref_nins_value_2;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, journal::hash("Side FS Ref INS Value 2")> side_fs_ref_ins_value_2;
    StoreItem<uint32_t, defaults::side_fs_value_span, journal::hash("Side FS Value Span 2")> side_fs_value_span_2;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, journal::hash("Side FS Ref Value 3")> side_fs_ref_nins_value_3;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, journal::hash("Side FS Ref INS Value 3")> side_fs_ref_ins_value_3;
    StoreItem<uint32_t, defaults::side_fs_value_span, journal::hash("Side FS Value Span 3")> side_fs_value_span_3;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, journal::hash("Side FS Ref Value 4")> side_fs_ref_nins_value_4;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, journal::hash("Side FS Ref INS Value 4")> side_fs_ref_ins_value_4;
    StoreItem<uint32_t, defaults::side_fs_value_span, journal::hash("Side FS Value Span 4")> side_fs_value_span_4;
    StoreItem<int32_t, defaults::side_fs_ref_nins_value, journal::hash("Side FS Ref Value 5")> side_fs_ref_nins_value_5;
    StoreItem<int32_t, defaults::side_fs_ref_ins_value, journal::hash("Side FS Ref INS Value 5")> side_fs_ref_ins_value_5;
    StoreItem<uint32_t, defaults::side_fs_value_span, journal::hash("Side FS Value Span 5")> side_fs_value_span_5;
#endif

#if HAS_MMU2()
    StoreItem<bool, defaults::bool_false, journal::hash("Is MMU Rework")> is_mmu_rework; // Indicates printer has been reworked for MMU (has a different FS behavior)
#endif

    StoreItem<side_fsensor_remap::Mapping, defaults::side_fs_remap, journal::hash("Side FS Remap")> side_fs_remap; ///< Side filament sensor remapping

    //// Helper array-like access functions for filament sensors
    int32_t get_extruder_fs_ref_nins_value(uint8_t index);
    int32_t get_extruder_fs_ref_ins_value(uint8_t index);
    void set_extruder_fs_ref_nins_value(uint8_t index, int32_t value);
    void set_extruder_fs_ref_ins_value(uint8_t index, int32_t value);
    uint32_t get_extruder_fs_value_span(uint8_t index);
    void set_extruder_fs_value_span(uint8_t index, uint32_t value);

#if HAS_SIDE_FSENSOR()
    int32_t get_side_fs_ref_nins_value(uint8_t index);
    int32_t get_side_fs_ref_ins_value(uint8_t index);
    void set_side_fs_ref_nins_value(uint8_t index, int32_t value);
    void set_side_fs_ref_ins_value(uint8_t index, int32_t value);
    uint32_t get_side_fs_value_span(uint8_t index);
    void set_side_fs_value_span(uint8_t index, uint32_t value);
#endif

    StoreItem<uint16_t, defaults::print_progress_time, journal::hash("Print Progress Time")> print_progress_time; // screen progress time in seconds
    StoreItem<bool, defaults::bool_true, journal::hash("TMC Wavetable Enabled")> tmc_wavetable_enabled; // wavetable in TMC drivers

#if HAS_MMU2()
    StoreItem<bool, defaults::bool_false, journal::hash("MMU2 Enabled")> mmu2_enabled;
    StoreItem<bool, defaults::bool_false, journal::hash("MMU2 Cutter")> mmu2_cutter; // use MMU2 cutter when it sees fit
    StoreItem<bool, defaults::bool_false, journal::hash("MMU2 Stealth Mode")> mmu2_stealth_mode; // run MMU2 in stealth mode wherever possible

    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("MMU2 load fails")> mmu2_load_fails;
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("MMU2 total load fails")> mmu2_total_load_fails;
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("MMU2 general fails")> mmu2_fails;
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("MMU2 total general fails")> mmu2_total_fails;
#endif

    StoreItem<bool, defaults::bool_true, journal::hash("Run LEDs")> run_leds;
    StoreItem<bool, defaults::bool_false, journal::hash("Heat Entire Bed")> heat_entire_bed;
    StoreItem<bool, defaults::bool_false, journal::hash("Touch Enabled")> touch_enabled;
    StoreItem<bool, defaults::bool_false, journal::hash("Touch Sig Workaround")> touch_sig_workaround;

#if HAS_TOOLCHANGER() // for now not ifdefing per-extruder as well for simplicity
    StoreItem<DockPosition, defaults::dock_position, journal::hash("Dock Position 0")> dock_position_0;
    StoreItem<DockPosition, defaults::dock_position, journal::hash("Dock Position 1")> dock_position_1;
    StoreItem<DockPosition, defaults::dock_position, journal::hash("Dock Position 2")> dock_position_2;
    StoreItem<DockPosition, defaults::dock_position, journal::hash("Dock Position 3")> dock_position_3;
    StoreItem<DockPosition, defaults::dock_position, journal::hash("Dock Position 4")> dock_position_4;
    StoreItem<DockPosition, defaults::dock_position, journal::hash("Dock Position 5")> dock_position_5;

    DockPosition get_dock_position(uint8_t index);
    void set_dock_position(uint8_t index, DockPosition value);

    StoreItem<ToolOffset, defaults::tool_offset, journal::hash("Tool Offset 0")> tool_offset_0;
    StoreItem<ToolOffset, defaults::tool_offset, journal::hash("Tool Offset 1")> tool_offset_1;
    StoreItem<ToolOffset, defaults::tool_offset, journal::hash("Tool Offset 2")> tool_offset_2;
    StoreItem<ToolOffset, defaults::tool_offset, journal::hash("Tool Offset 3")> tool_offset_3;
    StoreItem<ToolOffset, defaults::tool_offset, journal::hash("Tool Offset 4")> tool_offset_4;
    StoreItem<ToolOffset, defaults::tool_offset, journal::hash("Tool Offset 5")> tool_offset_5;

    ToolOffset get_tool_offset(uint8_t index);
    void set_tool_offset(uint8_t index, ToolOffset value);
#endif

    StoreItem<filament::Type, defaults::filament_type, journal::hash("Filament Type 0")> filament_type_0;
#if EXTRUDERS > 1 // for now only doing one ifdef for simplicity
    StoreItem<filament::Type, defaults::filament_type, journal::hash("Filament Type 1")> filament_type_1;
    StoreItem<filament::Type, defaults::filament_type, journal::hash("Filament Type 2")> filament_type_2;
    StoreItem<filament::Type, defaults::filament_type, journal::hash("Filament Type 3")> filament_type_3;
    StoreItem<filament::Type, defaults::filament_type, journal::hash("Filament Type 4")> filament_type_4;
    StoreItem<filament::Type, defaults::filament_type, journal::hash("Filament Type 5")> filament_type_5;
#endif

    filament::Type get_filament_type(uint8_t index);
    void set_filament_type(uint8_t index, filament::Type value);

    StoreItem<bool, defaults::bool_false, journal::hash("Heatup Bed")> heatup_bed;

    StoreItem<float, defaults::nozzle_diameter, journal::hash("Nozzle Diameter 0")> nozzle_diameter_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<float, defaults::nozzle_diameter, journal::hash("Nozzle Diameter 1")> nozzle_diameter_1;
    StoreItem<float, defaults::nozzle_diameter, journal::hash("Nozzle Diameter 2")> nozzle_diameter_2;
    StoreItem<float, defaults::nozzle_diameter, journal::hash("Nozzle Diameter 3")> nozzle_diameter_3;
    StoreItem<float, defaults::nozzle_diameter, journal::hash("Nozzle Diameter 4")> nozzle_diameter_4;
    StoreItem<float, defaults::nozzle_diameter, journal::hash("Nozzle Diameter 5")> nozzle_diameter_5;
#endif

    float get_nozzle_diameter(uint8_t index);
    void set_nozzle_diameter(uint8_t index, float value);

    StoreItem<float, defaults::float_zero, journal::hash("Homing Bump Divisor X")> homing_bump_divisor_x;
    StoreItem<float, defaults::float_zero, journal::hash("Homing Bump Divisor Y")> homing_bump_divisor_y;

    StoreItem<bool, defaults::bool_true, journal::hash("Enable Side LEDs")> side_leds_enabled;

    /// Whether the side leds should dim down a bit when user is not interacting with the printer or stay on full power the whole time
    StoreItem<bool, defaults::bool_true, journal::hash("Enable Side LEDs dimming")> side_leds_dimming_enabled;

    StoreItem<bool, defaults::bool_true, journal::hash("Enable Tool LEDs")> tool_leds_enabled;

    StoreItem<float, defaults::float_zero, journal::hash("Odometer X")> odometer_x;
    StoreItem<float, defaults::float_zero, journal::hash("Odometer Y")> odometer_y;
    StoreItem<float, defaults::float_zero, journal::hash("Odometer Z")> odometer_z;

    float get_odometer_axis(uint8_t index);
    void set_odometer_axis(uint8_t index, float value);

    StoreItem<float, defaults::float_zero, journal::hash("Odometer Extruded Length 0")> odometer_extruded_length_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<float, defaults::float_zero, journal::hash("Odometer Extruded Length 1")> odometer_extruded_length_1;
    StoreItem<float, defaults::float_zero, journal::hash("Odometer Extruded Length 2")> odometer_extruded_length_2;
    StoreItem<float, defaults::float_zero, journal::hash("Odometer Extruded Length 3")> odometer_extruded_length_3;
    StoreItem<float, defaults::float_zero, journal::hash("Odometer Extruded Length 4")> odometer_extruded_length_4;
    StoreItem<float, defaults::float_zero, journal::hash("Odometer Extruded Length 5")> odometer_extruded_length_5;
#endif

    float get_odometer_extruded_length(uint8_t index);
    void set_odometer_extruded_length(uint8_t index, float value);

    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Odometer Toolpicks 0")> odometer_toolpicks_0;
#if HOTENDS > 1 // for now only doing one ifdef for simplicity
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Odometer Toolpicks 1")> odometer_toolpicks_1;
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Odometer Toolpicks 2")> odometer_toolpicks_2;
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Odometer Toolpicks 3")> odometer_toolpicks_3;
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Odometer Toolpicks 4")> odometer_toolpicks_4;
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Odometer Toolpicks 5")> odometer_toolpicks_5;
#endif

    uint32_t get_odometer_toolpicks(uint8_t index);
    void set_odometer_toolpicks(uint8_t index, uint32_t value);

    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("MMU toolchanges")> mmu_changes;
    // Last time (in the mmu_changes) the user did maintenance
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Last MMU maintenance")> mmu_last_maintenance;
    // A "leaky bucket" for MMU failures.
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("MMU fail bucket")> mmu_fail_bucket;

    StoreItem<HWCheckSeverity, defaults::hw_check_severity, journal::hash("HW Check Nozzle")> hw_check_nozzle;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, journal::hash("HW Check Model")> hw_check_model;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, journal::hash("HW Check Firmware")> hw_check_firmware;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, journal::hash("HW Check G-code")> hw_check_gcode;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, journal::hash("HW Check Compatibility")> hw_check_compatibility;
#if HAS_SELFTEST()
    StoreItem<SelftestResult, defaults::selftest_result, journal::hash("Selftest Result Gears")> selftest_result;
#endif

#if PRINTER_IS_PRUSA_XL
    StoreItem<TestResult, defaults::test_result_unknown, journal::hash("Selftest Result - Nozzle Diameter")> selftest_result_nozzle_diameter;
#endif

#if PRINTER_IS_PRUSA_XL
    StoreItem<TestResult, defaults::test_result_unknown, journal::hash("Test Result Phase Stepping")> selftest_result_phase_stepping;
#endif

    SelftestTool get_selftest_result_tool(uint8_t index);
    void set_selftest_result_tool(uint8_t index, SelftestTool value);

    // TODO: create option/has_sheets and ifdef this next part with it
    StoreItem<uint8_t, defaults::uint8_t_zero, journal::hash("Active Sheet")> active_sheet;
    StoreItem<Sheet, defaults::sheet_0, journal::hash("Sheet 0")> sheet_0;
    StoreItem<Sheet, defaults::sheet_1, journal::hash("Sheet 1")> sheet_1;
    StoreItem<Sheet, defaults::sheet_2, journal::hash("Sheet 2")> sheet_2;
    StoreItem<Sheet, defaults::sheet_3, journal::hash("Sheet 3")> sheet_3;
    StoreItem<Sheet, defaults::sheet_4, journal::hash("Sheet 4")> sheet_4;
    StoreItem<Sheet, defaults::sheet_5, journal::hash("Sheet 5")> sheet_5;
    StoreItem<Sheet, defaults::sheet_6, journal::hash("Sheet 6")> sheet_6;
    StoreItem<Sheet, defaults::sheet_7, journal::hash("Sheet 7")> sheet_7;

    Sheet get_sheet(uint8_t index);
    void set_sheet(uint8_t index, Sheet value);

    // axis microsteps and rms current have a capital axis + '_' at the end in name because of trinamic.cpp. Can be removed once the macro there is removed
    StoreItem<float, defaults::axis_steps_per_unit_x, journal::hash("Axis Steps Per Unit X")> axis_steps_per_unit_x;
    StoreItem<float, defaults::axis_steps_per_unit_y, journal::hash("Axis Steps Per Unit Y")> axis_steps_per_unit_y;
    StoreItem<float, defaults::axis_steps_per_unit_z, journal::hash("Axis Steps Per Unit Z")> axis_steps_per_unit_z;
    StoreItem<float, defaults::axis_steps_per_unit_e0, journal::hash("Axis Steps Per Unit E0")> axis_steps_per_unit_e0;
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Axis Microsteps X")> axis_microsteps_X_; // 0 - depends on xy_motors_400_step, !=0 - user value independent on xy_motors_400_step
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Axis Microsteps Y")> axis_microsteps_Y_; // 0 - depends on xy_motors_400_step, !=0 - user value independent on xy_motors_400_step
    StoreItem<uint16_t, defaults::axis_microsteps_Z_, journal::hash("Axis Microsteps Z")> axis_microsteps_Z_;
    StoreItem<uint16_t, defaults::axis_microsteps_E0_, journal::hash("Axis Microsteps E0")> axis_microsteps_E0_;
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Axis RMS Current MA X")> axis_rms_current_ma_X_; // 0 - depends on xy_motors_400_step, !=0 - user value independent on xy_motors_400_step
    StoreItem<uint16_t, defaults::uint16_t_zero, journal::hash("Axis RMS Current MA Y")> axis_rms_current_ma_Y_; // 0 - depends on xy_motors_400_step, !=0 - user value independent on xy_motors_400_step
    StoreItem<uint16_t, defaults::axis_rms_current_ma_Z_, journal::hash("Axis RMS Current MA Z")> axis_rms_current_ma_Z_;
    StoreItem<uint16_t, defaults::axis_rms_current_ma_E0_, journal::hash("Axis RMS Current MA E0")> axis_rms_current_ma_E0_;
    StoreItem<float, defaults::axis_z_max_pos_mm, journal::hash("Axis Z Max Pos MM")> axis_z_max_pos_mm;

    // Nozzle Sock has is here for backwards compatibility (should be binary compatible)
    StoreItem<HotendType, defaults::hotend_type, journal::hash("Nozzle Sock")> hotend_type;
    StoreItem<NozzleType, defaults::nozzle_type, journal::hash("Nozzle Type")> nozzle_type;

    StoreItem<restore_z::Position, restore_z::default_position, journal::hash("Restore Z Coordinate After Boot")> restore_z_after_boot;

    StoreItem<int16_t, defaults::homing_sens_x, journal::hash("Homing Sens X")> homing_sens_x; // X axis homing sensitivity
    StoreItem<int16_t, defaults::homing_sens_y, journal::hash("Homing Sens Y")> homing_sens_y; // Y axis homing sensitivity

    StoreItem<bool, defaults::xy_motors_400_step, journal::hash("400 step motors on X and Y axis")> xy_motors_400_step;

    StoreItem<bool, defaults::bool_false, journal::hash("Stuck filament detection")> stuck_filament_detection;

    StoreItem<bool, defaults::bool_false, journal::hash("Stealth mode")> stealth_mode;

    StoreItem<bool, defaults::bool_true, journal::hash("Input Shaper Axis X Enabled")> input_shaper_axis_x_enabled;
    StoreItem<input_shaper::AxisConfig, input_shaper::axis_x_default, journal::hash("Input Shaper Axis X Config")> input_shaper_axis_x_config;
    StoreItem<bool, defaults::bool_true, journal::hash("Input Shaper Axis Y Enabled")> input_shaper_axis_y_enabled;
    StoreItem<input_shaper::AxisConfig, input_shaper::axis_y_default, journal::hash("Input Shaper Axis Y Config")> input_shaper_axis_y_config;
    StoreItem<bool, input_shaper::weight_adjust_enabled_default, journal::hash("Input Shaper Weight Adjust Y Enabled V2")> input_shaper_weight_adjust_y_enabled;
    StoreItem<input_shaper::WeightAdjustConfig, input_shaper::weight_adjust_y_default, journal::hash("Input Shaper Weight Adjust Y Config")> input_shaper_weight_adjust_y_config;

    input_shaper::Config get_input_shaper_config();
    void set_input_shaper_config(const input_shaper::Config &);

#if PRINTER_IS_PRUSA_MK3_5
    StoreItem<bool, defaults::bool_false, journal::hash("Has Alt Fans")> has_alt_fans;
#endif

#if HAS_PHASE_STEPPING()
    StoreItem<bool, defaults::bool_false, journal::hash("Phase Stepping Enabled X")> phase_stepping_enabled_x;
    StoreItem<bool, defaults::bool_false, journal::hash("Phase Stepping Enabled Y")> phase_stepping_enabled_y;

    bool get_phase_stepping_enabled(AxisEnum axis);
    void set_phase_stepping_enabled(AxisEnum axis, bool new_state);
#endif

#if PRINTER_IS_PRUSA_XL
    StoreItem<uint8_t, defaults::uint8_t_zero, journal::hash("XL Enclosure Flags")> xl_enclosure_flags;
    StoreItem<int64_t, defaults::int64_zero, journal::hash("XL Enclosure Filter Timer")> xl_enclosure_filter_timer;
    StoreItem<uint8_t, defaults::uint8_percentage_80, journal::hash("XL Enclosure Fan Manual Setting")> xl_enclosure_fan_manual;
#endif

#if PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_MINI
    StoreItem<int8_t, defaults::int8_t_zero, journal::hash("Left Bed Correction")> left_bed_correction;
    StoreItem<int8_t, defaults::int8_t_zero, journal::hash("Right Bed Correction")> right_bed_correction;
    StoreItem<int8_t, defaults::int8_t_zero, journal::hash("Front Bed Correction")> front_bed_correction;
    StoreItem<int8_t, defaults::int8_t_zero, journal::hash("Rear Bed Correction")> rear_bed_correction;
#endif
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
    // Selftest Result version before adding Gears Calibration result to EEPROM
    StoreItem<SelftestResult_pre_gears, defaults::selftest_result_pre_gears, journal::hash("Selftest Result V23")> selftest_result_pre_gears;

    // Changing Filament Sensor default state to remove necessity of FS dialog on startup
    StoreItem<bool, defaults::bool_true, journal::hash("FSensor Enabled")> fsensor_enabled_v1;

    // An item was added to the middle of the footer enum and it caused eeprom corruption. This store footer item  was deleted and a new one is created without migration so as to force default footer value onto everyone, which is better than 'random values' (especially on mini where it could cause duplicated items shown). Default value was removed since we no longer need to keep it
    StoreItem<uint32_t, defaults::uint32_t_zero, journal::hash("Footer Setting")> footer_setting_v1;

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

    // There was wrong default value for XL, so V2 version was introduced to reset it to proper default value
    StoreItem<bool, defaults::bool_true, journal::hash("Input Shaper Weight Adjust Y Enabled")> input_shaper_weight_adjust_y_enabled;
};

} // namespace config_store_ns
