// menu_vars.h - shared arrays to be used in menus
#pragma once
#include "stdint.h"
#include "../lang/i18n.h"

//-----------------------------------------------------------------------------
//stringize macros
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

extern const int32_t _noz_park[3];
extern const char *const gcode_nozzle_park;

#define z_offset_def nozzle_to_probe[2]

//If used with - Z safe homing is applyed
extern const char X_home_gcode[];
extern const char Y_home_gcode[];
extern const char Z_home_gcode[];

extern const int32_t filament_change_slow_load_length;
extern const int32_t filament_change_fast_load_length;
extern const int32_t filament_change_full_load_length;
extern const int32_t filament_change_slow_purge_length;
extern const int32_t filament_change_full_purge_load_length;
extern const float filament_unload_mini_length;

#include <array>
#include <cstdint>
using std::size_t;
struct MenuVars {
    constexpr static const size_t AXIS_CNT = 4;
    constexpr static const size_t RANGE_SZ = 3;
    constexpr static const char *const zoffset_prt_format = "%.3f";
    constexpr static const char *const labels[] = { N_("Move X"), N_("Move Y"), N_("Move Z"), N_("Move E") };
    static const std::array<std::array<int16_t, RANGE_SZ>, AXIS_CNT> axis_ranges;
    static const int16_t manual_feedrate[AXIS_CNT];
    static const char axis_letters[AXIS_CNT];
    static const int16_t extrude_min_temp;

    static const std::array<uint16_t, RANGE_SZ> nozzle_range;
    static const std::array<uint8_t, RANGE_SZ> bed_range;
    static const std::array<uint8_t, RANGE_SZ> printfan_range;
    static const std::array<uint16_t, RANGE_SZ> flowfact_range;
    static const std::array<uint16_t, RANGE_SZ> feedrate_range;
    static const std::array<float, RANGE_SZ> zoffset_fl_range;

private:
    MenuVars() = delete;
};
