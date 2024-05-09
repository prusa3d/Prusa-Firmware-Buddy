// menu_vars.h - shared arrays to be used in menus
#pragma once
#include "i18n.h"
#include "file_list_defs.h"

//-----------------------------------------------------------------------------
// stringize macros
#define QUOTE_ME(x) #x
#define STR(x)      QUOTE_ME(x)

// axis length [mm]
extern const int x_axis_len;
extern const int y_axis_len;
extern const int z_axis_len;

// tolerance (common for all axes)
extern const int len_tol_abs; // length absolute tolerance (+-5mm)
extern const int len_tol_rev; // length tolerance in reversed direction (3mm)

extern const int axis_steps_per_unit[];

extern const float nozzle_to_probe[3];

extern const float z_offset_step;
extern const float z_offset_min;
extern const float z_offset_max;

extern const int _noz_park[3];
extern const char *const gcode_nozzle_park;

#define z_offset_def nozzle_to_probe[2]

extern const int default_Z_max_pos;

// Z is loaded from eeprom, cannot be used
extern const char X_home_gcode[];
extern const char Y_home_gcode[];

// shared buffer where gui stores name of currently selected or printed file
extern char gui_media_LFN[FILE_NAME_BUFFER_LEN];
extern char gui_media_SFN_path[FILE_PATH_BUFFER_LEN];

#include <array>
#include <cstdint>
using std::size_t;
struct MenuVars {
    constexpr static const size_t AXIS_CNT = 4;
    constexpr static const size_t RANGE_SZ = 3;
    constexpr static const char *const zoffset_prt_format = "%.3f";
    constexpr static const char *const labels[] = { N_("Move X"), N_("Move Y"), N_("Move Z"), N_("Move E") };

    constexpr static std::array<int, RANGE_SZ> printfan_range = { 0, 255, 1 };
    constexpr static std::array<int, RANGE_SZ> percent_range = { 0, 100, 1 };
    constexpr static std::array<int, RANGE_SZ> flowfact_range = { 50, 150, 1 };
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX)
    constexpr static std::array<int, RANGE_SZ> feedrate_range = { 50, 1000, 1 };
#else
    constexpr static std::array<int, RANGE_SZ> feedrate_range = { 10, 255, 1 };
#endif
    constexpr static std::array<int, MenuVars::RANGE_SZ> microstep_exponential_range = { 1, 256, 2 }; // 2^0 - 2^8 .. 1, 2, 4, .. , 128, 256
    constexpr static std::array<int, MenuVars::RANGE_SZ> microstep_exponential_range_with_0 = { 0, 256, 2 }; // 0 + 2^0 - 2^8 .. 0, 1, 2, 4, .. , 128, 256
    constexpr static std::array<int, MenuVars::RANGE_SZ> axis_rms_currents_range = { 0, 800, 1 };
    constexpr static std::array<int, MenuVars::RANGE_SZ> steps_per_unit_range = { 1, 1000, 1 }; // small range, experimental feature
#if XL_ENCLOSURE_SUPPORT()
    constexpr static std::array<int, RANGE_SZ> enclosure_fan_percent_range = { 50, 100, 10 };
    constexpr static std::array<int, RANGE_SZ> enclosure_post_print_minute_range = { 1, 10, 1 };
#endif

    static const std::array<int, AXIS_CNT> GetManualFeedrate();
    static const std::array<char, AXIS_CNT> GetAxisLetters();

    static const std::array<int, RANGE_SZ> GetNozzleRange();
    static const std::array<int, RANGE_SZ> GetBedRange();
    static const std::array<int, RANGE_SZ> GetMaximumZRange();
    static const std::array<std::array<int, RANGE_SZ>, AXIS_CNT> GetAxisRanges();

private:
    MenuVars() = delete;
};
