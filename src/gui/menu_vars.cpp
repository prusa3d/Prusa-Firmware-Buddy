#include "menu_vars.h"
#include "config.h"
#include "int_to_cstr.h"

#include "../Marlin/src/inc/MarlinConfig.h"
#include "../Marlin/src/module/temperature.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "gui_config_mini.h"
    #include "Configuration_A3ides_2209_MINI_adv.h"
#else
    #error "Unknown PRINTER_TYPE."
#endif

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

//must be in this file, need to access marlin
constexpr const int park_points[3] = NOZZLE_PARK_POINT;

//min int -2147483648 .. 8 digits + 1 for /0

constexpr const int X_home = X_HOME_DIR > 0 ? X_MAX_POS : X_MIN_POS;
constexpr const int Y_home = Y_HOME_DIR > 0 ? Y_MAX_POS : Y_MIN_POS;
constexpr const int Z_home = Z_HOME_DIR > 0 ? Z_MAX_POS : Z_MIN_POS;

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

constexpr const char Z_home_gcode[] = {
    'G',
    '9',
    '2',
    ' ',
    'Z',
    nth_char(Z_home, 0),
    nth_char(Z_home, 1),
    nth_char(Z_home, 2),
    nth_char(Z_home, 3),
    nth_char(Z_home, 4),
    nth_char(Z_home, 5),
    nth_char(Z_home, 6),
    nth_char(Z_home, 7),
    nth_char(Z_home, 8)
};

const std::array<std::array<int16_t, MenuVars::RANGE_SZ>, MenuVars::AXIS_CNT> MenuVars::axis_ranges = { { { X_MIN_POS, X_MAX_POS, 1 },
    { Y_MIN_POS, Y_MAX_POS, 1 },
    { Z_MIN_POS, Z_MAX_POS, 1 },
    { -EXTRUDE_MAXLENGTH, EXTRUDE_MAXLENGTH, 1 } } };
const int16_t MenuVars::manual_feedrate[AXIS_CNT] = MANUAL_FEEDRATE;
const char MenuVars::axis_letters[AXIS_CNT] = { 'X', 'Y', 'Z', 'E' };
const int16_t MenuVars::extrude_min_temp = EXTRUDE_MINTEMP;

const std::array<uint16_t, MenuVars::RANGE_SZ> MenuVars::nozzle_range = { 0, (HEATER_0_MAXTEMP - 15), 1 };
const std::array<uint8_t, MenuVars::RANGE_SZ> MenuVars::bed_range = { 0, (BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN), 1 };
const std::array<uint8_t, MenuVars::RANGE_SZ> MenuVars::printfan_range = { 0, 255, 1 };
const std::array<uint16_t, MenuVars::RANGE_SZ> MenuVars::flowfact_range = { 50, 150, 1 };
const std::array<uint16_t, MenuVars::RANGE_SZ> MenuVars::feedrate_range = { 10, 255, 1 };
const std::array<float, MenuVars::RANGE_SZ> MenuVars::zoffset_fl_range = { z_offset_min, z_offset_max, z_offset_step };

constexpr const int32_t filament_change_slow_load_length = FILAMENT_CHANGE_SLOW_LOAD_LENGTH;
constexpr const int32_t filament_change_fast_load_length = FILAMENT_CHANGE_FAST_LOAD_LENGTH;
constexpr const int32_t filament_change_slow_purge_length = 40;
constexpr const float filament_unload_mini_length = 392.0F;

constexpr const int32_t filament_change_full_load_length = filament_change_fast_load_length + filament_change_slow_load_length;
constexpr const int32_t filament_change_full_purge_load_length = filament_change_full_load_length + filament_change_slow_purge_length;
