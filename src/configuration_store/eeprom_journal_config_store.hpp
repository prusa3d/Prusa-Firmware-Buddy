#pragma once
#include "eeprom_journal.hpp"
#include "../../lib/Marlin/Marlin/src/feature/input_shaper/input_shaper_config.hpp"
#include "journal/configuration_store.hpp"
#include <module/temperature.h>
#include <config.h>
#include <eeprom.h>
#include <sound_enum.h>
#include <footer_eeprom.hpp>
#include <time_tools.hpp>
#include <filament.hpp>

// TODO: Find a better home for this
inline bool operator==(DockPosition lhs, DockPosition rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

// TODO: Find a better home for this
inline bool operator==(ToolOffset lhs, ToolOffset rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

// TODO: Find a better home for this
inline bool operator==(SelftestResult lhs, SelftestResult rhs) {
    for (size_t i = 0; i < std::size(lhs.tools); ++i) {
        if (lhs.tools[i].dockoffset != rhs.tools[i].dockoffset
            || lhs.tools[i].fsensor != rhs.tools[i].fsensor
            || lhs.tools[i].sideFsensor != rhs.tools[i].sideFsensor
            || lhs.tools[i].heatBreakFan != rhs.tools[i].heatBreakFan
            || lhs.tools[i].printFan != rhs.tools[i].printFan
            || lhs.tools[i].loadcell != rhs.tools[i].loadcell
            || lhs.tools[i].nozzle != rhs.tools[i].nozzle
            || lhs.tools[i].tooloffset != rhs.tools[i].tooloffset) {
            return false;
        }
    }
    return lhs.bed == rhs.bed
        && lhs.eth == rhs.eth
        && lhs.wifi == rhs.wifi
        && lhs.xaxis == rhs.xaxis
        && lhs.yaxis == rhs.yaxis
        && lhs.zaxis == rhs.zaxis
        && lhs.zalign == rhs.zalign;
}

// TODO: Find a better home for this
inline bool operator==(Sheet lhs, Sheet rhs) {
    for (size_t i = 0; i < std::size(lhs.name); ++i) {
        if (lhs.name[i] != rhs.name[i]) {
            return false;
        }
        if (lhs.name[i] == '\0') {
            break;
        }
    }
    return lhs.z_offset == rhs.z_offset;
}

// TODO: Find a better home for this
enum class HWCheckSeverity : uint8_t {
    Ignore = 0,
    Warning = 1,
    Abort = 2
};

namespace eeprom_journal {

// Holds default constants so they can be referenced by store item. Placing these constants in another header where it's more meaningful is welcome. These defaults could be passed directly as template parameter to store items from gcc 11 onwards (and store items would accept them as value instead of as a const ref).
namespace defaults {
    // Variables without a distinct default values can use these shared ones
    inline constexpr bool bool_true { true };
    inline constexpr bool bool_false { false };

    inline constexpr float float_zero { 0.0f };
    inline constexpr uint8_t uint8_t_zero { 0 };
    inline constexpr uint16_t uint16_t_zero { 0 };
    inline constexpr uint32_t uint32_t_zero { 0 };

    // default values for variables that have distinct requirements
    inline constexpr float pid_nozzle_p {
#ifdef DEFAULT_Kp
        DEFAULT_Kp
#else
        0.0f
#endif
    };
    inline constexpr float pid_nozzle_i {
#ifdef DEFAULT_Ki
        scalePID_i(DEFAULT_Ki)
#else
        0.0f
#endif
    };
    inline constexpr float pid_nozzle_d {
#ifdef DEFAULT_Kd
        scalePID_d(DEFAULT_Kd)
#else
        0.0f
#endif
    };

    inline constexpr float pid_bed_p {
#ifdef DEFAULT_bedKp
        DEFAULT_bedKp
#else
        0.0f
#endif
    };
    inline constexpr float pid_bed_i {
#ifdef DEFAULT_bedKi
        scalePID_i(DEFAULT_bedKi)
#else
        0.0f
#endif
    };
    inline constexpr float pid_bed_d {
#ifdef DEFAULT_bedKd
        scalePID_d(DEFAULT_bedKd)
#else
        0.0f
#endif
    };

    inline constexpr std::array<char, LAN_HOSTNAME_MAX_LEN + 1> net_hostname { DEFAULT_HOST_NAME };
    inline constexpr int8_t lan_timezone { 1 };
    inline constexpr std::array<char, WIFI_MAX_SSID_LEN + 1> wifi_ap_ssid { "" };
    inline constexpr std::array<char, WIFI_MAX_PASSWD_LEN + 1> wifi_ap_password { "" };

    inline constexpr eSOUND_MODE sound_mode { eSOUND_MODE::UNDEF };
    inline constexpr uint8_t sound_volume { 5 };
    inline constexpr uint16_t language { 0xffff };
    inline constexpr uint32_t footer_setting { footer::eeprom::Encode(footer::DefaultItems) };
    inline constexpr uint32_t footer_draw_type { footer::ItemDrawCnf::Default() };
    inline constexpr std::array<char, PL_PASSWORD_SIZE> prusalink_password { "" };

    inline constexpr std::array<char, CONNECT_HOST_SIZE + 1> connect_host { "buddy-a.\x01\x01" }; // "Compressed" - this means buddy-a.connect.prusa3d.com.
    inline constexpr std::array<char, CONNECT_TOKEN_SIZE + 1> connect_token { "" };
    inline constexpr uint16_t connect_port { 443 };

    inline constexpr bool crash_enabled {
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5)
        false
#else
        true
#endif // (( PRINTER_IS_PRUSA_MK4) || ( PRINTER_IS_PRUSA_MK3_5))
    };

    inline constexpr int16_t crash_sens[2] =
#if ENABLED(CRASH_RECOVERY)
        CRASH_STALL_GUARD;
#else
        { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

    inline constexpr int16_t crash_sens_x { crash_sens[0] };
    inline constexpr int16_t crash_sens_y { crash_sens[1] };

    static constexpr uint16_t crash_max_period[2] =
#if ENABLED(CRASH_RECOVERY)
        CRASH_MAX_PERIOD;
#else
        { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

    inline constexpr uint16_t crash_max_period_x { crash_max_period[0] };
    inline constexpr uint16_t crash_max_period_y { crash_max_period[1] };

    inline constexpr bool crash_filter {
#if ENABLED(CRASH_RECOVERY)
        CRASH_FILTER
#else
        false
#endif // ENABLED(CRASH_RECOVERY)
    };

    inline constexpr time_format::TF_t time_format { time_format::TF_t::TF_24H };

    inline constexpr float loadcell_scale { 0.0192f };
    inline constexpr float loadcell_threshold_static { -125.f };
    inline constexpr float loadcell_hysteresis { 80.f };
    inline constexpr float loadcell_threshold_continuous { -40.f };

    inline constexpr int32_t extruder_fs_ref_value { std::numeric_limits<int32_t>::min() }; // min == will require calibration
    inline constexpr uint32_t extruder_fs_value_span {
#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100)
        100
#elif (BOARD_IS_XLBUDDY)
        1000
#else
        350000
#endif
    };

    inline constexpr int32_t side_fs_ref_value { std::numeric_limits<int32_t>::min() }; // min == will require calibration
    inline constexpr uint32_t side_fs_value_span { 310 };

    inline constexpr uint16_t print_progress_time { 30 };

    inline constexpr DockPosition dock_position { 0, 0 };
    inline constexpr ToolOffset tool_offset { 0, 0, 0 };

    inline constexpr filament::Type filament_type { filament::Type::NONE };
    inline constexpr float nozzle_diameter {
#if PRINTER_IS_PRUSA_XL
        0.60f
#else
        0.40f
#endif
    };

    inline constexpr HWCheckSeverity hw_check_severity { HWCheckSeverity::Warning };
    inline constexpr SelftestResult selftest_result {};

    inline constexpr Sheet sheet_0 { "Smooth1", 0.0f };
    inline constexpr Sheet sheet_1 { "Smooth2", eeprom_z_offset_uncalibrated };
    inline constexpr Sheet sheet_2 { "Textur1", eeprom_z_offset_uncalibrated };
    inline constexpr Sheet sheet_3 { "Textur2", eeprom_z_offset_uncalibrated };
    inline constexpr Sheet sheet_4 { "Custom1", eeprom_z_offset_uncalibrated };
    inline constexpr Sheet sheet_5 { "Custom2", eeprom_z_offset_uncalibrated };
    inline constexpr Sheet sheet_6 { "Custom3", eeprom_z_offset_uncalibrated };
    inline constexpr Sheet sheet_7 { "Custom4", eeprom_z_offset_uncalibrated };

    inline constexpr float default_axis_steps_flt[4] = DEFAULT_AXIS_STEPS_PER_UNIT;
    inline constexpr float axis_steps_per_unit_x { default_axis_steps_flt[0] * ((DEFAULT_INVERT_X_DIR == true) ? -1.f : 1.f) };
    inline constexpr float axis_steps_per_unit_y { default_axis_steps_flt[1] * ((DEFAULT_INVERT_Y_DIR == true) ? -1.f : 1.f) };
    inline constexpr float axis_steps_per_unit_z { default_axis_steps_flt[2] * ((DEFAULT_INVERT_Z_DIR == true) ? -1.f : 1.f) };
    inline constexpr float axis_steps_per_unit_e0 { default_axis_steps_flt[3] * ((DEFAULT_INVERT_E0_DIR == true) ? -1.f : 1.f) };
    inline constexpr uint16_t axis_microsteps_X_ { X_MICROSTEPS };
    inline constexpr uint16_t axis_microsteps_Y_ { Y_MICROSTEPS };
    inline constexpr uint16_t axis_microsteps_Z_ { Z_MICROSTEPS };
    inline constexpr uint16_t axis_microsteps_E0_ { E0_MICROSTEPS };
    inline constexpr uint16_t axis_rms_current_ma_X_ { X_CURRENT };
    inline constexpr uint16_t axis_rms_current_ma_Y_ { Y_CURRENT };
    inline constexpr uint16_t axis_rms_current_ma_Z_ { Z_CURRENT };
    inline constexpr uint16_t axis_rms_current_ma_E0_ { E0_CURRENT };
    inline constexpr float axis_z_max_pos_mm {
#ifdef DEFAULT_Z_MAX_POS
        DEFAULT_Z_MAX_POS
#else
        0
#endif
    };
}

/**
 * @brief Holds all current store items -> there is a RAM mirror of this data which is loaded upon device restart from eeprom.

 * !! HASHES CANNOT BE CHANGED !!
 * This HASH cannot be the same as an already existing one (there is a compiler check to ensure this).
 * !! NEVER JUST DELETE AN ITEM FROM THIS STRUCT; if an item is no longer wanted, deprecate it. See DeprecatedStore (below).
 * !! Changing DEFAULT VALUE is ALSO a deprecation !!
 */
struct CurrentStore : public Journal::CurrentStoreConfig<Journal::Backend, backend> {
    // wizard flags
    StoreItem<bool, defaults::bool_true, Journal::hash("Run Selftest")> run_selftest;
    StoreItem<bool, defaults::bool_true, Journal::hash("Run XYZ Calibration")> run_xyz_calib;
    StoreItem<bool, defaults::bool_true, Journal::hash("Run First Layer")> run_first_layer;

    StoreItem<bool, defaults::bool_true, Journal::hash("FSensor Enabled")> fsensor_enabled;

    // nozzle PID variables
    StoreItem<float, defaults::pid_nozzle_p, Journal::hash("PID Nozzle P")> pid_nozzle_p;
    StoreItem<float, defaults::pid_nozzle_i, Journal::hash("PID Nozzle I")> pid_nozzle_i;
    StoreItem<float, defaults::pid_nozzle_d, Journal::hash("PID Nozzle D")> pid_nozzle_d;

    // bed PID variables
    StoreItem<float, defaults::pid_bed_p, Journal::hash("PID Bed P")> pid_bed_p;
    StoreItem<float, defaults::pid_bed_i, Journal::hash("PID Bed I")> pid_bed_i;
    StoreItem<float, defaults::pid_bed_d, Journal::hash("PID Bed D")> pid_bed_d;

    // LAN settings
    // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1
    StoreItem<uint8_t, defaults::uint8_t_zero, Journal::hash("LAN Flag")> lan_flag;
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("LAN IP4 Address")> lan_ip4_addr;    // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("LAN IP4 Mask")> lan_ip4_mask;       // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("LAN IP4 Gateway")> lan_ip4_gateway; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("LAN IP4 DNS1")> lan_ip4_dns1;       // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("LAN IP4 DNS2")> lan_ip4_dns2;       // X.X.X.X address encoded
    StoreItem<std::array<char, LAN_HOSTNAME_MAX_LEN + 1>, defaults::net_hostname, Journal::hash("LAN Hostname")> lan_hostname;

    StoreItem<int8_t, defaults::lan_timezone, Journal::hash("LAN Timezone")> timezone; // hour difference from UTC

    // WIFI settings
    // wifi_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1, wifi_flag & 0b1100 -> reserved, previously ap_sec_t security
    StoreItem<uint8_t, defaults::uint8_t_zero, Journal::hash("WIFI Flag")> wifi_flag;
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("WIFI IP4 Address")> wifi_ip4_addr;    // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("WIFI IP4 Mask")> wifi_ip4_mask;       // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("WIFI IP4 Gateway")> wifi_ip4_gateway; // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("WIFI IP4 DNS1")> wifi_ip4_dns1;       // X.X.X.X address encoded
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("WIFI IP4 DNS2")> wifi_ip4_dns2;       // X.X.X.X address encoded
    StoreItem<std::array<char, LAN_HOSTNAME_MAX_LEN + 1>, defaults::net_hostname, Journal::hash("WIFI Hostname")> wifi_hostname;
    StoreItem<std::array<char, WIFI_MAX_SSID_LEN + 1>, defaults::wifi_ap_ssid, Journal::hash("WIFI AP SSID")> wifi_ap_ssid;
    StoreItem<std::array<char, WIFI_MAX_PASSWD_LEN + 1>, defaults::wifi_ap_password, Journal::hash("WIFI AP Password")> wifi_ap_password;

    StoreItem<eSOUND_MODE, defaults::sound_mode, Journal::hash("Sound Mode")> sound_mode;
    StoreItem<uint8_t, defaults::sound_volume, Journal::hash("Sound Volume")> sound_volume;
    StoreItem<uint16_t, defaults::language, Journal::hash("Language")> language;
    StoreItem<uint8_t, defaults::uint8_t_zero, Journal::hash("File Sort")> file_sort;   // filebrowser file sort options
    StoreItem<bool, defaults::bool_true, Journal::hash("Menu Timeout")> menu_timeout;   // on / off menu timeout flag
    StoreItem<bool, defaults::bool_true, Journal::hash("Devhash in QR")> devhash_in_qr; // on / off sending UID in QR
    StoreItem<uint32_t, defaults::footer_setting, Journal::hash("Footer Setting")> footer_setting;
    StoreItem<uint32_t, defaults::footer_draw_type, Journal::hash("Footer Draw Type")> footer_draw_type;
    StoreItem<bool, defaults::bool_true, Journal::hash("Fan Check Enabled")> fan_check_enabled;
    StoreItem<bool, defaults::bool_true, Journal::hash("FS Autoload Enabled")> fs_autoload_enabled;

    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("Odometer Time")> odometer_time;
    StoreItem<uint8_t, defaults::uint8_t_zero, Journal::hash("Active NetDev")> active_netdev; // active network device
    StoreItem<bool, defaults::bool_true, Journal::hash("PrusaLink Enabled")> prusalink_enabled;
    StoreItem<std::array<char, PL_PASSWORD_SIZE>, defaults::prusalink_password, Journal::hash("PrusaLink Password")> prusalink_password;

    StoreItem<bool, defaults::bool_false, Journal::hash("USB MSC Enabled")> usb_msc_enabled;

    StoreItem<std::array<char, CONNECT_HOST_SIZE + 1>, defaults::connect_host, Journal::hash("Connect Host")> connect_host;
    StoreItem<std::array<char, CONNECT_TOKEN_SIZE + 1>, defaults::connect_token, Journal::hash("Connect Token")> connect_token;
    StoreItem<uint16_t, defaults::connect_port, Journal::hash("Connect Port")> connect_port;
    StoreItem<bool, defaults::bool_true, Journal::hash("Connect TLS")> connect_tls;
    StoreItem<bool, defaults::bool_false, Journal::hash("Connect Enabled")> connect_enabled;

    StoreItem<uint16_t, defaults::uint16_t_zero, Journal::hash("Job ID")> job_id; // print job id incremented at every print start

    StoreItem<bool, defaults::crash_enabled, Journal::hash("Crash Enabled")> crash_enabled;
    StoreItem<int16_t, defaults::crash_sens_x, Journal::hash("Crash Sens X")> crash_sens_x; // X axis crash sensitivity
    StoreItem<int16_t, defaults::crash_sens_y, Journal::hash("Crash Sens Y")> crash_sens_y; // Y axis crash sensitivity

    // X axis max crash period (speed) threshold
    StoreItem<uint16_t, defaults::crash_max_period_x, Journal::hash("Crash Sens Max Period X")> crash_max_period_x;
    // Y axis max crash period (speed) threshold
    StoreItem<uint16_t, defaults::crash_max_period_y, Journal::hash("Crash Sens Max Period Y")> crash_max_period_y;
    StoreItem<bool, defaults::crash_filter, Journal::hash("Crash Filter")> crash_filter;                  // Stallguard filtration
    StoreItem<uint16_t, defaults::uint16_t_zero, Journal::hash("Crash Count X")> crash_count_x;           // number of crashes of X axis in total
    StoreItem<uint16_t, defaults::uint16_t_zero, Journal::hash("Crash Count Y")> crash_count_y;           // number of crashes of Y axis in total
    StoreItem<uint16_t, defaults::uint16_t_zero, Journal::hash("Power Panics Count")> power_panics_count; // number of power losses in total

    StoreItem<time_format::TF_t, defaults::time_format, Journal::hash("Time Format")> time_format;

    StoreItem<float, defaults::loadcell_scale, Journal::hash("Loadcell Scale")> loadcell_scale;
    StoreItem<float, defaults::loadcell_threshold_static, Journal::hash("Loadcell Threshold Static")> loadcell_threshold_static;
    StoreItem<float, defaults::loadcell_hysteresis, Journal::hash("Loadcell Hysteresis")> loadcell_hysteresis;
    StoreItem<float, defaults::loadcell_threshold_continuous, Journal::hash("Loadcell Threshold Continuous")> loadcell_threshold_continuous;

    // filament sensor values:
    // ref value: value of filament sensor in moment of calibration (w/o filament present)
    // value span: minimal difference of raw values between the two states of the filament sensor
    StoreItem<int32_t, defaults::extruder_fs_ref_value, Journal::hash("Extruder FS Ref Value 0")> extruder_fs_ref_value_0;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, Journal::hash("Extruder FS Value Span 0")> extruder_fs_value_span_0;
    StoreItem<int32_t, defaults::extruder_fs_ref_value, Journal::hash("Extruder FS Ref Value 1")> extruder_fs_ref_value_1;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, Journal::hash("Extruder FS Value Span 1")> extruder_fs_value_span_1;
    StoreItem<int32_t, defaults::extruder_fs_ref_value, Journal::hash("Extruder FS Ref Value 2")> extruder_fs_ref_value_2;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, Journal::hash("Extruder FS Value Span 2")> extruder_fs_value_span_2;
    StoreItem<int32_t, defaults::extruder_fs_ref_value, Journal::hash("Extruder FS Ref Value 3")> extruder_fs_ref_value_3;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, Journal::hash("Extruder FS Value Span 3")> extruder_fs_value_span_3;
    StoreItem<int32_t, defaults::extruder_fs_ref_value, Journal::hash("Extruder FS Ref Value 4")> extruder_fs_ref_value_4;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, Journal::hash("Extruder FS Value Span 4")> extruder_fs_value_span_4;
    StoreItem<int32_t, defaults::extruder_fs_ref_value, Journal::hash("Extruder FS Ref Value 5")> extruder_fs_ref_value_5;
    StoreItem<uint32_t, defaults::extruder_fs_value_span, Journal::hash("Extruder FS Value Span 5")> extruder_fs_value_span_5;

    StoreItem<int32_t, defaults::side_fs_ref_value, Journal::hash("Side FS Ref Value 0")> side_fs_ref_value_0;
    StoreItem<uint32_t, defaults::side_fs_value_span, Journal::hash("Side FS Value Span 0")> side_fs_value_span_0;
    StoreItem<int32_t, defaults::side_fs_ref_value, Journal::hash("Side FS Ref Value 1")> side_fs_ref_value_1;
    StoreItem<uint32_t, defaults::side_fs_value_span, Journal::hash("Side FS Value Span 1")> side_fs_value_span_1;
    StoreItem<int32_t, defaults::side_fs_ref_value, Journal::hash("Side FS Ref Value 2")> side_fs_ref_value_2;
    StoreItem<uint32_t, defaults::side_fs_value_span, Journal::hash("Side FS Value Span 2")> side_fs_value_span_2;
    StoreItem<int32_t, defaults::side_fs_ref_value, Journal::hash("Side FS Ref Value 3")> side_fs_ref_value_3;
    StoreItem<uint32_t, defaults::side_fs_value_span, Journal::hash("Side FS Value Span 3")> side_fs_value_span_3;
    StoreItem<int32_t, defaults::side_fs_ref_value, Journal::hash("Side FS Ref Value 4")> side_fs_ref_value_4;
    StoreItem<uint32_t, defaults::side_fs_value_span, Journal::hash("Side FS Value Span 4")> side_fs_value_span_4;
    StoreItem<int32_t, defaults::side_fs_ref_value, Journal::hash("Side FS Ref Value 5")> side_fs_ref_value_5;
    StoreItem<uint32_t, defaults::side_fs_value_span, Journal::hash("Side FS Value Span 5")> side_fs_value_span_5;

    //// Helper array-like access functions for filament sensors
    int32_t get_extruder_fs_ref_value(uint8_t index);
    void set_extruder_fs_ref_value(uint8_t index, int32_t value);
    uint32_t get_extruder_fs_value_span(uint8_t index);
    void set_extruder_fs_value_span(uint8_t index, uint32_t value);

    int32_t get_side_fs_ref_value(uint8_t index);
    void set_side_fs_ref_value(uint8_t index, int32_t value);
    uint32_t get_side_fs_value_span(uint8_t index);
    void set_side_fs_value_span(uint8_t index, uint32_t value);

    StoreItem<uint16_t, defaults::print_progress_time, Journal::hash("Print Progress Time")> print_progress_time; // screen progress time in seconds
    StoreItem<bool, defaults::bool_true, Journal::hash("TMC Wavetable Enabled")> tmc_wavetable_enabled;           // wavetable in TMC drivers

    StoreItem<bool, defaults::bool_false, Journal::hash("MMU2 Enabled")> mmu2_enabled;
    StoreItem<bool, defaults::bool_false, Journal::hash("MMU2 Cutter")> mmu2_cutter;             // use MMU2 cutter when it sees fit
    StoreItem<bool, defaults::bool_false, Journal::hash("MMU2 Stealth Mode")> mmu2_stealth_mode; // run MMU2 in stealth mode wherever possible

    StoreItem<bool, defaults::bool_true, Journal::hash("Run LEDs")> run_leds;
    StoreItem<bool, defaults::bool_false, Journal::hash("Heat Entire Bed")> heat_entire_bed;
    StoreItem<bool, defaults::bool_false, Journal::hash("Touch Enabled")> touch_enabled;

    StoreItem<DockPosition, defaults::dock_position, Journal::hash("Dock Position 0")> dock_position_0;
    StoreItem<DockPosition, defaults::dock_position, Journal::hash("Dock Position 1")> dock_position_1;
    StoreItem<DockPosition, defaults::dock_position, Journal::hash("Dock Position 2")> dock_position_2;
    StoreItem<DockPosition, defaults::dock_position, Journal::hash("Dock Position 3")> dock_position_3;
    StoreItem<DockPosition, defaults::dock_position, Journal::hash("Dock Position 4")> dock_position_4;
    StoreItem<DockPosition, defaults::dock_position, Journal::hash("Dock Position 5")> dock_position_5;

    DockPosition get_dock_position(uint8_t index);
    void set_dock_position(uint8_t index, DockPosition value);

    StoreItem<ToolOffset, defaults::tool_offset, Journal::hash("Tool Offset 0")> tool_offset_0;
    StoreItem<ToolOffset, defaults::tool_offset, Journal::hash("Tool Offset 1")> tool_offset_1;
    StoreItem<ToolOffset, defaults::tool_offset, Journal::hash("Tool Offset 2")> tool_offset_2;
    StoreItem<ToolOffset, defaults::tool_offset, Journal::hash("Tool Offset 3")> tool_offset_3;
    StoreItem<ToolOffset, defaults::tool_offset, Journal::hash("Tool Offset 4")> tool_offset_4;
    StoreItem<ToolOffset, defaults::tool_offset, Journal::hash("Tool Offset 5")> tool_offset_5;

    ToolOffset get_tool_offset(uint8_t index);
    void set_tool_offset(uint8_t index, ToolOffset value);

    StoreItem<filament::Type, defaults::filament_type, Journal::hash("Filament Type 0")> filament_type_0;
    StoreItem<filament::Type, defaults::filament_type, Journal::hash("Filament Type 1")> filament_type_1;
    StoreItem<filament::Type, defaults::filament_type, Journal::hash("Filament Type 2")> filament_type_2;
    StoreItem<filament::Type, defaults::filament_type, Journal::hash("Filament Type 3")> filament_type_3;
    StoreItem<filament::Type, defaults::filament_type, Journal::hash("Filament Type 4")> filament_type_4;
    StoreItem<filament::Type, defaults::filament_type, Journal::hash("Filament Type 5")> filament_type_5;

    filament::Type get_filament_type(uint8_t index);
    void set_filament_type(uint8_t index, filament::Type value);

    StoreItem<bool, defaults::bool_false, Journal::hash("Heatup Bed")> heatup_bed;

    StoreItem<float, defaults::nozzle_diameter, Journal::hash("Nozzle Diameter 0")> nozzle_diameter_0;
    StoreItem<float, defaults::nozzle_diameter, Journal::hash("Nozzle Diameter 1")> nozzle_diameter_1;
    StoreItem<float, defaults::nozzle_diameter, Journal::hash("Nozzle Diameter 2")> nozzle_diameter_2;
    StoreItem<float, defaults::nozzle_diameter, Journal::hash("Nozzle Diameter 3")> nozzle_diameter_3;
    StoreItem<float, defaults::nozzle_diameter, Journal::hash("Nozzle Diameter 4")> nozzle_diameter_4;
    StoreItem<float, defaults::nozzle_diameter, Journal::hash("Nozzle Diameter 5")> nozzle_diameter_5;

    float get_nozzle_diameter(uint8_t index);
    void set_nozzle_diameter(uint8_t index, float value);

    StoreItem<float, defaults::float_zero, Journal::hash("Homing Bump Divisor X")> homing_bump_divisor_x;
    StoreItem<float, defaults::float_zero, Journal::hash("Homing Bump Divisor Y")> homing_bump_divisor_y;

    StoreItem<bool, defaults::bool_true, Journal::hash("Enable Side LEDs")> side_leds_enabled;
    StoreItem<bool, defaults::bool_true, Journal::hash("Enable Tool LEDs")> tool_leds_enabled;

    StoreItem<float, defaults::float_zero, Journal::hash("Odometer X")> odometer_x;
    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Y")> odometer_y;
    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Z")> odometer_z;

    float get_odometer_axis(uint8_t index);
    void set_odometer_axis(uint8_t index, float value);

    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Extruded Length 0")> odometer_extruded_length_0;
    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Extruded Length 1")> odometer_extruded_length_1;
    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Extruded Length 2")> odometer_extruded_length_2;
    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Extruded Length 3")> odometer_extruded_length_3;
    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Extruded Length 4")> odometer_extruded_length_4;
    StoreItem<float, defaults::float_zero, Journal::hash("Odometer Extruded Length 5")> odometer_extruded_length_5;

    float get_odometer_extruded_length(uint8_t index);
    void set_odometer_extruded_length(uint8_t index, float value);

    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("Odometer Toolpicks 0")> odometer_toolpicks_0;
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("Odometer Toolpicks 1")> odometer_toolpicks_1;
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("Odometer Toolpicks 2")> odometer_toolpicks_2;
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("Odometer Toolpicks 3")> odometer_toolpicks_3;
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("Odometer Toolpicks 4")> odometer_toolpicks_4;
    StoreItem<uint32_t, defaults::uint32_t_zero, Journal::hash("Odometer Toolpicks 5")> odometer_toolpicks_5;

    uint32_t get_odometer_toolpicks(uint8_t index);
    void set_odometer_toolpicks(uint8_t index, uint32_t value);

    StoreItem<HWCheckSeverity, defaults::hw_check_severity, Journal::hash("HW Check Nozzle")> hw_check_nozzle;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, Journal::hash("HW Check Model")> hw_check_model;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, Journal::hash("HW Check Firmware")> hw_check_firmware;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, Journal::hash("HW Check G-code")> hw_check_gcode;
    StoreItem<HWCheckSeverity, defaults::hw_check_severity, Journal::hash("HW Check Compatibility")> hw_check_compatibility;

    StoreItem<SelftestResult, defaults::selftest_result, Journal::hash("Selftest Result")> selftest_result;

    SelftestTool get_selftest_result_tool(uint8_t index);
    void set_selftest_result_tool(uint8_t index, SelftestTool value);

    StoreItem<uint8_t, defaults::uint8_t_zero, Journal::hash("Active Sheet")> active_sheet;
    StoreItem<Sheet, defaults::sheet_0, Journal::hash("Sheet 0")> sheet_0;
    StoreItem<Sheet, defaults::sheet_1, Journal::hash("Sheet 1")> sheet_1;
    StoreItem<Sheet, defaults::sheet_2, Journal::hash("Sheet 2")> sheet_2;
    StoreItem<Sheet, defaults::sheet_3, Journal::hash("Sheet 3")> sheet_3;
    StoreItem<Sheet, defaults::sheet_4, Journal::hash("Sheet 4")> sheet_4;
    StoreItem<Sheet, defaults::sheet_5, Journal::hash("Sheet 5")> sheet_5;
    StoreItem<Sheet, defaults::sheet_6, Journal::hash("Sheet 6")> sheet_6;
    StoreItem<Sheet, defaults::sheet_7, Journal::hash("Sheet 7")> sheet_7;

    Sheet get_sheet(uint8_t index);
    void set_sheet(uint8_t index, Sheet value);

    // axis microsteps and rms current have a capital axis + '_' at the end in name because of trinamic.cpp. Can be removed once that macro there is removed
    StoreItem<float, defaults::axis_steps_per_unit_x, Journal::hash("Axis Steps Per Unit X")> axis_steps_per_unit_x;
    StoreItem<float, defaults::axis_steps_per_unit_y, Journal::hash("Axis Steps Per Unit Y")> axis_steps_per_unit_y;
    StoreItem<float, defaults::axis_steps_per_unit_z, Journal::hash("Axis Steps Per Unit Z")> axis_steps_per_unit_z;
    StoreItem<float, defaults::axis_steps_per_unit_e0, Journal::hash("Axis Steps Per Unit E0")> axis_steps_per_unit_e0;
    StoreItem<uint16_t, defaults::axis_microsteps_X_, Journal::hash("Axis Microsteps X")> axis_microsteps_X_;
    StoreItem<uint16_t, defaults::axis_microsteps_Y_, Journal::hash("Axis Microsteps Y")> axis_microsteps_Y_;
    StoreItem<uint16_t, defaults::axis_microsteps_Z_, Journal::hash("Axis Microsteps Z")> axis_microsteps_Z_;
    StoreItem<uint16_t, defaults::axis_microsteps_E0_, Journal::hash("Axis Microsteps E0")> axis_microsteps_E0_;
    StoreItem<uint16_t, defaults::axis_rms_current_ma_X_, Journal::hash("Axis RMS Current MA X")> axis_rms_current_ma_X_;
    StoreItem<uint16_t, defaults::axis_rms_current_ma_Y_, Journal::hash("Axis RMS Current MA Y")> axis_rms_current_ma_Y_;
    StoreItem<uint16_t, defaults::axis_rms_current_ma_Z_, Journal::hash("Axis RMS Current MA Z")> axis_rms_current_ma_Z_;
    StoreItem<uint16_t, defaults::axis_rms_current_ma_E0_, Journal::hash("Axis RMS Current MA E0")> axis_rms_current_ma_E0_;
    StoreItem<float, defaults::axis_z_max_pos_mm, Journal::hash("Axis Z Max Pos MM")> axis_z_max_pos_mm;

    StoreItem<bool, defaults::bool_false, Journal::hash("Nozzle Sock")> nozzle_sock;
    StoreItem<uint8_t, defaults::uint8_t_zero, Journal::hash("Nozzle Type")> nozzle_type;

    StoreItem<bool, defaults::bool_true, Journal::hash("Input Shaper Axis X Enabled")> input_shaper_axis_x_enabled;
    StoreItem<input_shaper::AxisConfig, input_shaper::axis_x_default, Journal::hash("Input Shaper Axis X Config")> input_shaper_axis_x_config;
    StoreItem<bool, defaults::bool_true, Journal::hash("Input Shaper Axis Y Enabled")> input_shaper_axis_y_enabled;
    StoreItem<input_shaper::AxisConfig, input_shaper::axis_y_default, Journal::hash("Input Shaper Axis Y Config")> input_shaper_axis_y_config;
    StoreItem<bool, defaults::bool_true, Journal::hash("Input Shaper Weight Adjust Y Enabled")> input_shaper_weight_adjust_y_enabled;
    StoreItem<input_shaper::WeightAdjustConfig, input_shaper::weight_adjust_y_default, Journal::hash("Input Shaper Weight Adjust Y Config")> input_shaper_weight_adjust_y_config;

    input_shaper::Config get_input_shaper_config();
    void set_input_shaper_config(const input_shaper::Config &);
};

/**
 * @brief Holds all deprecated store items. To deprecate an item, move it from CurrentStore to this DeprecatedStore, and change type (including parameters) accordingly.
 *
 * Right now there are two available deprecation strategies, with helper types in the base class:
 *  - Delete an item -> DeletedStoreItem. If an journal entry with this HASH is found in journal, it is considered valid state, but this journal entry will be discarded
 *  - Migrate to a newer StoreItem -> DeprecatedStoreItem. One of the parameters is a member pointer to the succeeding CurentStore::StoreItem. Upon encountering this journal entry, it is read as the deprecated (old) item, then converted to the new item via constructor of the new item which accepts old type as parameter. The succeeding CurentStore::StoreItem has to have a different HASHED ID than one deprecated (ie successor to hash("Sound Mode") could be hash("Sound Mode V2"))
 *
 * !!! MAKE SURE StoreItems moved from current store to here KEEP their HASHED ID !!!
 */
struct DeprecatedStore : public Journal::DeprecatedStoreConfig<Journal::Backend> {
};

enum class InitResult {
    migrated_from_old,
    cold_start,
    normal,
    not_yet_init
};

}

/**
 * @brief Instance of ConfigStore journal with journal strategy and EEPROM backend. Currently there is only one config store in the entire project, hence why the 'simple' "config_store" name
 *
 */
inline decltype(auto) config_store() {
    return Journal::journal<eeprom_journal::CurrentStore, eeprom_journal::DeprecatedStore>();
}

inline eeprom_journal::InitResult &config_store_init_result() {
    static eeprom_journal::InitResult init_result { eeprom_journal::InitResult::not_yet_init };
    return init_result;
}
