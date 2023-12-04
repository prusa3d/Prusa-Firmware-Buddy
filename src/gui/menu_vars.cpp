#include "menu_vars.h"
#include "config.h"
#include "int_to_cstr.h"

#include "../Marlin/src/module/temperature.h"

#include "gui_config_printer.hpp"
#include "SteelSheets.hpp"

const int x_axis_len = X_LEN;
const int y_axis_len = Y_LEN;
const int z_axis_len = Z_LEN;

// tolerance (common for all axes)
const int len_tol_abs = LEN_TOL_ABS; // length absolute tolerance (+-5mm)
const int len_tol_rev = LEN_TOL_REV; // length tolerance in reversed direction (3mm)

const int axis_steps_per_unit[] = DEFAULT_AXIS_STEPS_PER_UNIT;

const float nozzle_to_probe[3] = NOZZLE_TO_PROBE_OFFSET;

const float z_offset_step = 1.0F / float(axis_steps_per_unit[2]);
const float z_offset_min = Z_OFFSET_MIN;
const float z_offset_max = Z_OFFSET_MAX;

// must be in this file, need to access marlin
#if ENABLED(NOZZLE_PARK_FEATURE)
constexpr const int park_points[3] = NOZZLE_PARK_POINT;
#endif

constexpr const int default_Z_max_pos = DEFAULT_Z_MAX_POS;

// min int -2147483648 .. 8 digits + 1 for /0
constexpr const int X_home = X_HOME_DIR > 0 ? X_MAX_POS : X_MIN_POS;
constexpr const int Y_home = Y_HOME_DIR > 0 ? Y_MAX_POS : Y_MIN_POS;

constexpr const char X_home_gcode[] = {
    'G',
    '9',
    '2',
    ' ',
    'X',
    nth_char(X_home, 0),
    nth_char(X_home, 1),
    nth_char(X_home, 2),
    nth_char(X_home, 3),
    nth_char(X_home, 4),
    nth_char(X_home, 5),
    nth_char(X_home, 6),
    nth_char(X_home, 7),
    nth_char(X_home, 8)
};

constexpr const char Y_home_gcode[] = {
    'G',
    '9',
    '2',
    ' ',
    'Y',
    nth_char(Y_home, 0),
    nth_char(Y_home, 1),
    nth_char(Y_home, 2),
    nth_char(Y_home, 3),
    nth_char(Y_home, 4),
    nth_char(Y_home, 5),
    nth_char(Y_home, 6),
    nth_char(Y_home, 7),
    nth_char(Y_home, 8)
};

const std::array<int, MenuVars::RANGE_SZ> MenuVars::GetMaximumZRange() { return { { Z_MIN_LEN_LIMIT, Z_MAX_LEN_LIMIT, 1 } }; };
const std::array<std::array<int, MenuVars::RANGE_SZ>, MenuVars::AXIS_CNT> MenuVars::GetAxisRanges() {
    return { {
#if PRINTER_IS_PRUSA_XL
        // restrict movement of the tool for user to the bed area only to prevent crashes of the tool at toolchange area
        { X_MIN_POS, X_MAX_POS, 1 },
        { Y_MIN_POS, Y_MAX_PRINT_POS, 1 },
#else
        { X_MIN_POS, X_MAX_POS, 1 },
        { Y_MIN_POS, Y_MAX_POS, 1 },
#endif
        { static_cast<int>(std::lround(Z_MIN_POS)), static_cast<int>(get_z_max_pos_mm_rounded()), 1 },
        { -EXTRUDE_MAXLENGTH, EXTRUDE_MAXLENGTH, 1 } } };
};

const std::array<int, MenuVars::AXIS_CNT> MenuVars::GetManualFeedrate() { return { MANUAL_FEEDRATE }; };
const std::array<char, MenuVars::AXIS_CNT> MenuVars::GetAxisLetters() { return { 'X', 'Y', 'Z', 'E' }; };

const std::array<int, MenuVars::RANGE_SZ> MenuVars::GetNozzleRange() { return { 0, (HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN), 1 }; };
const std::array<int, MenuVars::RANGE_SZ> MenuVars::GetBedRange() { return { 0, (BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN), 1 }; };

#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX || PRINTER_IS_PRUSA_XL)
constexpr const int filament_change_slow_load_length = 45;
constexpr const int filament_change_fast_load_length = 40;
constexpr const int filament_change_slow_purge_length = 30;
constexpr const float filament_unload_mini_length = 80.0F;
#else
constexpr const int filament_change_slow_load_length = FILAMENT_CHANGE_SLOW_LOAD_LENGTH;
constexpr const int filament_change_fast_load_length = FILAMENT_CHANGE_FAST_LOAD_LENGTH;
constexpr const int filament_change_slow_purge_length = 40;
constexpr const float filament_unload_mini_length = 392.0F;
#endif

constexpr const int filament_change_full_load_length = filament_change_fast_load_length + filament_change_slow_load_length;
constexpr const int filament_change_full_purge_load_length = filament_change_full_load_length + filament_change_slow_purge_length;
