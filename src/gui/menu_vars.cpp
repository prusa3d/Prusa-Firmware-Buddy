#include "menu_vars.h"
#include "config.h"
#include "int_to_cstr.h"

#include "../Marlin/src/module/temperature.h"

#include "gui_config_printer.hpp"
#include "SteelSheets.hpp"

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
const std::array<int, MenuVars::RANGE_SZ> MenuVars::GetCrashSensitivity() {
#if AXIS_DRIVER_TYPE_X(TMC2209)
    return { 0, 255, 1 };
#elif AXIS_DRIVER_TYPE_X(TMC2130)
    return { -64, 63, 1 };
#else
    #error "Unknown driver type."
#endif
}
const std::array<int, MenuVars::RANGE_SZ> MenuVars::GetNozzleRange() { return { 0, (HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN), 1 }; };
const std::array<int, MenuVars::RANGE_SZ> MenuVars::GetBedRange() { return { 0, (BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN), 1 }; };
