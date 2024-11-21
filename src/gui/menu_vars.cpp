#include "menu_vars.h"
#include "config.h"
#include "int_to_cstr.h"

#include "../Marlin/src/module/temperature.h"

#include "gui_config_printer.hpp"

const std::pair<int, int> MenuVars::crash_sensitivity_range = {
#if AXIS_DRIVER_TYPE_X(TMC2209)
    0, 255
#elif AXIS_DRIVER_TYPE_X(TMC2130)
    -64, 63
#else
    #error "Unknown driver type."
#endif
};

std::pair<int, int> MenuVars::axis_range(uint8_t axis) {
    switch (axis) {
    case X_AXIS:
        return { X_MIN_POS, X_MAX_POS };

    case Y_AXIS:
#if PRINTER_IS_PRUSA_XL()
        // restrict movement of the tool for user to the bed area only to prevent crashes of the tool at toolchange area
        return { Y_MIN_POS, Y_MAX_PRINT_POS };
#else
        return { Y_MIN_POS, Y_MAX_POS };
#endif

    case Z_AXIS:
        return { static_cast<int>(std::lround(Z_MIN_POS)), static_cast<int>(get_z_max_pos_mm_rounded()) };

    case E_AXIS:
        return { -EXTRUDE_MAXLENGTH, EXTRUDE_MAXLENGTH };

    default:
        return { 0, 0 };
    }
}
