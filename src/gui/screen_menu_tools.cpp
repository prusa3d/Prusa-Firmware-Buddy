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
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_INFO_DWARF_MCU_TEMPERATURE
/*****************************************************************************/
MI_INFO_DWARF_MCU_TEMPERATURE::MI_INFO_DWARF_MCU_TEMPERATURE()
    : WI_TEMP_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_INFO_HEATBREAK_N_TEMP
I_MI_INFO_HEATBREAK_N_TEMP::I_MI_INFO_HEATBREAK_N_TEMP(const char *const specific_label, int index)
    : WI_TEMP_LABEL_t(prusa_toolchanger.is_toolchanger_enabled() ? _(specific_label) : _(generic_label), //< Toolchanger has specific labels
        nullptr, is_enabled_t::yes,
        ((index == 0) || (prusa_toolchanger.is_toolchanger_enabled() && buddy::puppies::dwarfs[index].is_enabled())) ? is_hidden_t::no : is_hidden_t::yes) { //< Index 0 is never hidden
}

/*****************************************************************************/
// MI_INFO_NOZZLE_N_TEMP
I_MI_INFO_NOZZLE_N_TEMP::I_MI_INFO_NOZZLE_N_TEMP(const char *const specific_label, int index)
    : WI_TEMP_LABEL_t(prusa_toolchanger.is_toolchanger_enabled() ? _(specific_label) : _(generic_label), //< Toolchanger has specific labels
        nullptr, is_enabled_t::yes,
        ((index == 0) || (prusa_toolchanger.is_toolchanger_enabled() && buddy::puppies::dwarfs[index].is_enabled())) ? is_hidden_t::no : is_hidden_t::yes) { //< Index 0 is never hidden
}

MI_ODOMETER_DIST_E::MI_ODOMETER_DIST_E(const char *const label, int index)
    : MI_ODOMETER_DIST(_(label), nullptr, is_enabled_t::yes,
        prusa_toolchanger.is_toolchanger_enabled() && prusa_toolchanger.is_tool_enabled(index) ? is_hidden_t::no : is_hidden_t::yes, -1) {
}

MI_ODOMETER_TOOL::MI_ODOMETER_TOOL(const char *const label, int index)
    : WI_FORMATABLE_LABEL_t<uint32_t>(_(label), nullptr, is_enabled_t::yes,
        prusa_toolchanger.is_toolchanger_enabled() && prusa_toolchanger.is_tool_enabled(index) ? is_hidden_t::no : is_hidden_t::yes, 0,
        [&](char *buffer) {
            snprintf(buffer, GuiDefaults::infoDefaultLen, "%lu %s", value, times_label);
        }) {
}

MI_ODOMETER_TOOL::MI_ODOMETER_TOOL()
    : MI_ODOMETER_TOOL(generic_label, 0) {
}
