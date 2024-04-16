#pragma once

#include <module/temperature.h>
#include <config.h>
#include <sound_enum.h>
#include <footer_eeprom.hpp>
#include <time_tools.hpp>
#include <filament.hpp>
#include <selftest_result.hpp>
#include <option/development_items.h>
#include <common/sheet.hpp>
#include <module/prusa/dock_position.hpp>
#include <module/prusa/tool_offset.hpp>
#include <filament_sensors_remap_data.hpp>
#include <printers.h>

#include "constants.hpp"
#include <common/nozzle_type.hpp>
#include <common/hotend_type.hpp>

#include <option/has_sheet_support.h>
#include <option/has_loadcell.h>

namespace config_store_ns {

// Holds default constants so they can be referenced by store item. Placing these constants in another header where it's more meaningful is welcome. These defaults could be passed directly as template parameter to store items from gcc 11 onwards (and store items would accept them as value instead of as a const ref).
namespace defaults {
    // Variables without a distinct default values can use these shared ones
    inline constexpr bool bool_true { true };
    inline constexpr bool bool_false { false };

    inline constexpr float float_zero { 0.0f };
    inline constexpr uint8_t uint8_t_zero { 0 };
    inline constexpr uint16_t uint16_t_zero { 0 };
    inline constexpr uint32_t uint32_t_zero { 0 };
    inline constexpr int8_t int8_t_zero { 0 };

    inline constexpr uint8_t uint8_t_ff { 0xff };

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

    inline constexpr TestResult test_result_unknown { TestResult_Unknown };

    inline constexpr std::array<char, lan_hostname_max_len + 1> net_hostname { LAN_HOSTNAME_DEF };
    inline constexpr int8_t lan_timezone { 1 };
    inline constexpr time_tools::TimezoneOffsetMinutes timezone_minutes { time_tools::TimezoneOffsetMinutes::no_offset };
    inline constexpr time_tools::TimezoneOffsetSummerTime timezone_summer { time_tools::TimezoneOffsetSummerTime::no_summertime };
    inline constexpr std::array<char, wifi_max_ssid_len + 1> wifi_ap_ssid { "" };
    inline constexpr std::array<char, wifi_max_passwd_len + 1> wifi_ap_password { "" };

    inline constexpr eSOUND_MODE sound_mode { eSOUND_MODE::_undef };
    inline constexpr uint8_t sound_volume { 5 };
    inline constexpr uint16_t language { 0xffff };

    inline constexpr footer::Item footer_setting_0 {
#if FOOTER_ITEMS_PER_LINE__ > 0
        footer::default_items[0]
#else
        footer::Item::none
#endif
    };
    inline constexpr footer::Item footer_setting_1 {
#if FOOTER_ITEMS_PER_LINE__ > 1
        footer::default_items[1]
#else
        footer::Item::none
#endif
    };
    inline constexpr footer::Item footer_setting_2 {
#if FOOTER_ITEMS_PER_LINE__ > 2
        footer::default_items[2]
#else
        footer::Item::none
#endif
    };
    inline constexpr footer::Item footer_setting_3 {
#if FOOTER_ITEMS_PER_LINE__ > 3
        footer::default_items[3]
#else
        footer::Item::none
#endif
    };
    inline constexpr footer::Item footer_setting_4 {
#if FOOTER_ITEMS_PER_LINE__ > 4
        footer::default_items[4]
#else
        footer::Item::none
#endif
    };

    inline constexpr uint32_t footer_draw_type { footer::ItemDrawCnf::get_default() };
    inline constexpr std::array<char, pl_password_size> prusalink_password { "" };

    inline constexpr std::array<char, connect_host_size + 1> connect_host { "buddy-a.\x01\x01" }; // "Compressed" - this means buddy-a.connect.prusa3d.com.
    inline constexpr std::array<char, connect_token_size + 1> connect_token { "" };
    inline constexpr uint16_t connect_port { 443 };

    // Production build need user to intentionally allow them
    inline constexpr MetricsAllow metrics_allow { MetricsAllow::None };
    inline constexpr std::array<char, metrics_host_size + 1> metrics_host { "" };
    inline constexpr bool metrics_init { false };
    inline constexpr uint16_t metrics_port { 8514 };
    inline constexpr uint16_t syslog_port { 13514 };

    inline constexpr bool crash_enabled {
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_XL)
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

    inline constexpr time_tools::TimeFormat time_format { time_tools::TimeFormat::_24h };

    inline constexpr float loadcell_scale { 0.0192f };
    inline constexpr float loadcell_threshold_static { -125.f };
    inline constexpr float loadcell_hysteresis { 80.f };
    inline constexpr float loadcell_threshold_continuous { -40.f };

    // Filament sensor reference NOT INSERTED value
    inline constexpr int32_t extruder_fs_ref_nins_value { std::numeric_limits<int32_t>::min() }; // min == will require calibration
    // Filament sensor reference INSERTED value
    // min == invalid value
    // Note that previous FW versions didn't save this value during calibration, so fsensor has to work with this default value
    inline constexpr int32_t extruder_fs_ref_ins_value { std::numeric_limits<int32_t>::min() };

    inline constexpr uint32_t extruder_fs_value_span {
#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100)
        100
#elif (BOARD_IS_XLBUDDY)
        1000
#else
        350000
#endif
    };

    // SIDE Filament sensor reference NOT INSERTED value
    // min == will require calibration
    inline constexpr int32_t side_fs_ref_nins_value { std::numeric_limits<int32_t>::min() };
    // SIDE Filament sensor reference INSERTED value
    // min == invalid value
    // Note that previous FW versions didn't save this value during calibration, so fsensor has to work with this default value
    inline constexpr int32_t side_fs_ref_ins_value { std::numeric_limits<int32_t>::min() };
    inline constexpr uint32_t side_fs_value_span { 310 };

    inline constexpr bool fsensor_enabled {
#if PRINTER_IS_PRUSA_MINI || PRINTER_IS_PRUSA_MK3_5
        true // MINI and 3.5 do not require any calibration
#else
        false
#endif
    };

    inline constexpr side_fsensor_remap::Mapping side_fs_remap = side_fsensor_remap::preset::no_mapping; // No remapping

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
    inline constexpr SelftestResult_pre_gears selftest_result_pre_gears {};
    inline constexpr SelftestResult_pre_23 selftest_result_pre_23 {};

#if (HAS_SHEET_SUPPORT())
    static_assert(!HAS_LOADCELL(), "This caused major issues on XL.");
    // Sheet[0] is used both as default sheet and as storage for z offset set by LiveAdjust Z. XL have had and issue that caused the tool offset calibration to fail due to aforementioned z offset. Uncalibrated value being float:max caused z offset to be set to 2 mm which lead to printer missing the calibration pin by almost 2 whole mm. This happens to all XLs that do not have a LiveAdjust Z value set and therefore use the default value.
    inline constexpr Sheet sheet_0 { "Smooth1", z_offset_uncalibrated };
#else
    inline constexpr Sheet sheet_0 { "Smooth1", 0.0f };
#endif
    inline constexpr Sheet sheet_1 { "Smooth2", z_offset_uncalibrated };
    inline constexpr Sheet sheet_2 { "Textur1", z_offset_uncalibrated };
    inline constexpr Sheet sheet_3 { "Textur2", z_offset_uncalibrated };
    inline constexpr Sheet sheet_4 { "Custom1", z_offset_uncalibrated };
    inline constexpr Sheet sheet_5 { "Custom2", z_offset_uncalibrated };
    inline constexpr Sheet sheet_6 { "Custom3", z_offset_uncalibrated };
    inline constexpr Sheet sheet_7 { "Custom4", z_offset_uncalibrated };

    inline constexpr float default_axis_steps_flt[4] = DEFAULT_AXIS_STEPS_PER_UNIT;
    inline constexpr float axis_steps_per_unit_x { default_axis_steps_flt[0] * ((DEFAULT_INVERT_X_DIR == true) ? -1.f : 1.f) };
    inline constexpr float axis_steps_per_unit_y { default_axis_steps_flt[1] * ((DEFAULT_INVERT_Y_DIR == true) ? -1.f : 1.f) };
    inline constexpr float axis_steps_per_unit_z { default_axis_steps_flt[2] * ((DEFAULT_INVERT_Z_DIR == true) ? -1.f : 1.f) };
    inline constexpr float axis_steps_per_unit_e0 { default_axis_steps_flt[3] * ((DEFAULT_INVERT_E0_DIR == true) ? -1.f : 1.f) };
    inline constexpr uint16_t axis_microsteps_Z_ { Z_MICROSTEPS };
    inline constexpr uint16_t axis_microsteps_E0_ { E0_MICROSTEPS };
    inline constexpr uint16_t axis_rms_current_ma_Z_ { Z_CURRENT };
    inline constexpr uint16_t axis_rms_current_ma_E0_ { E0_CURRENT };
    inline constexpr float axis_z_max_pos_mm {
#ifdef DEFAULT_Z_MAX_POS
        DEFAULT_Z_MAX_POS
#else
        0
#endif
    };

    inline constexpr int16_t homing_sens_x { stallguard_sensitivity_unset };
    inline constexpr int16_t homing_sens_y { stallguard_sensitivity_unset };

    inline constexpr bool xy_motors_400_step {
#if PRINTER_IS_PRUSA_MK4
        true
#else
        false
#endif
    };
    inline constexpr HotendType hotend_type {
#if PRINTER_IS_PRUSA_iX
        HotendType::stock_with_sock
#else
        HotendType::stock
#endif
    };
    inline constexpr NozzleType nozzle_type {
        NozzleType::Normal
    };
    inline constexpr uint8_t uint8_percentage_80 { 80 };
    inline constexpr int64_t int64_zero { 0 };
} // namespace defaults

} // namespace config_store_ns
