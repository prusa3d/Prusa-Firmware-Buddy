/**
 * @file screen_menu_tools.hpp
 * @brief This is temporary menu enabling dock position and tool offset view and edit. Simple manual calibration of the dock position is included.
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "selftest_frame.hpp"
#include "MItem_hardware.hpp"

class MI_INFO_DWARF_BOARD_TEMPERATURE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_DWARF_BOARD_TEMPERATURE();
};

class MI_INFO_DWARF_MCU_TEMPERATURE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_DWARF_MCU_TEMPERATURE();
};

/**
 * @brief Tool-specific odometer item.
 * @param OdometerT class with constructor that takes N and label.
 * @param N which extruder [indexed from 0]
 */
template <class OdometerT, int N>
class MI_ODOMETER_N : public OdometerT {
    static_assert(N >= 0 && N <= 4, "bad input");
    static consteval const char *get_name() {
        switch (N) {
        case 0:
            return N_("  Tool 1"); // Keep space in front for menu alignment
        case 1:
            return N_("  Tool 2");
        case 2:
            return N_("  Tool 3");
        case 3:
            return N_("  Tool 4");
        case 4:
            return N_("  Tool 5");
        }
        consteval_assert_false();
        return "";
    }

    static constexpr const char *const specific_label = get_name();

public:
    MI_ODOMETER_N()
        : OdometerT(specific_label, N) {
    }
};

// Specializations of odometer display for particular tool
template <int N>
using MI_ODOMETER_DIST_E_N = MI_ODOMETER_N<MI_ODOMETER_DIST_E, N>;
template <int N>
using MI_ODOMETER_TOOL_N = MI_ODOMETER_N<MI_ODOMETER_TOOL, N>;
