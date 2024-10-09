/**
 * @file screen_menu_tools.hpp
 * @brief This is temporary menu enabling dock position and tool offset view and edit. Simple manual calibration of the dock position is included.
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "selftest_frame.hpp"
#include "MItem_hardware.hpp"
#include "WindowItemTempLabel.hpp"

class MI_INFO_DWARF_BOARD_TEMPERATURE : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("Dwarf Board Temp");

public:
    MI_INFO_DWARF_BOARD_TEMPERATURE();
};

class MI_INFO_DWARF_MCU_TEMPERATURE : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("Dwarf MCU Temp");

public:
    MI_INFO_DWARF_MCU_TEMPERATURE();
};

class I_MI_INFO_NOZZLE_N_TEMP : public WI_TEMP_LABEL_t {
    static constexpr const char *const generic_label = N_("Nozzle Temperature"); // Generic string for single tool version

public:
    I_MI_INFO_NOZZLE_N_TEMP(const char *const specific_label, int index);
};

template <int N>
class MI_INFO_NOZZLE_N_TEMP : public I_MI_INFO_NOZZLE_N_TEMP {
    static_assert(N >= 0 && N <= 4, "bad input");
    static consteval const char *get_name() {
        switch (N) {
        case 0:
            return N_("Nozzle 1 Temperature");
        case 1:
            return N_("Nozzle 2 Temperature");
        case 2:
            return N_("Nozzle 3 Temperature");
        case 3:
            return N_("Nozzle 4 Temperature");
        case 4:
            return N_("Nozzle 5 Temperature");
        }
        consteval_assert_false();
        return "";
    }

    static constexpr const char *const specific_label = get_name();

public:
    MI_INFO_NOZZLE_N_TEMP()
        : I_MI_INFO_NOZZLE_N_TEMP(specific_label, N) {
    }
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
