/**
 * @file screen_menu_tools.cpp
 */

#include "screen_menu_tools.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"

#include "module/motion.h"
#include "module/prusa/toolchanger.h"
#include "marlin_client.hpp"

#include <puppies/Dwarf.hpp>
#include <limits>
#include <config_store/store_instance.hpp>

/*****************************************************************************/
// MI_INFO_DWARF_BOARD_TEMPERATURE
/*****************************************************************************/
MI_INFO_DWARF_BOARD_TEMPERATURE::MI_INFO_DWARF_BOARD_TEMPERATURE()
    : MenuItemAutoUpdatingLabel(_("Dwarf Board Temp"), standard_print_format::temp_c,
        [] { return sensor_data().dwarfBoardTemperature; } //
    ) {}

/*****************************************************************************/
// MI_INFO_DWARF_MCU_TEMPERATURE
/*****************************************************************************/
MI_INFO_DWARF_MCU_TEMPERATURE::MI_INFO_DWARF_MCU_TEMPERATURE()
    : MenuItemAutoUpdatingLabel(_("Dwarf MCU Temp"), standard_print_format::temp_c,
        [] { return sensor_data().dwarfMCUTemperature; } //
    ) {}

MI_ODOMETER_DIST_E::MI_ODOMETER_DIST_E(const char *const label, int index)
    : MI_ODOMETER_DIST(_(label), nullptr, is_enabled_t::yes,
        prusa_toolchanger.is_toolchanger_enabled() && prusa_toolchanger.is_tool_enabled(index) ? is_hidden_t::no : is_hidden_t::yes, -1) {
}

MI_ODOMETER_TOOL::MI_ODOMETER_TOOL(const char *const label, int index)
    : WI_FORMATABLE_LABEL_t<uint32_t>(_(label), nullptr, is_enabled_t::yes,
        prusa_toolchanger.is_toolchanger_enabled() && prusa_toolchanger.is_tool_enabled(index) ? is_hidden_t::no : is_hidden_t::yes, 0,
        [&](const std::span<char> &buffer) {
            snprintf(buffer.data(), buffer.size(), "%lu %s", value, times_label);
        }) {
}

MI_ODOMETER_TOOL::MI_ODOMETER_TOOL()
    : MI_ODOMETER_TOOL(generic_label, 0) {
}
