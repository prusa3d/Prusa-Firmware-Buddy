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

static int displayed_tool = 0;

static constexpr NumericInputConfig OFFSET_CONFIG_X {
    .min_value = X_MIN_OFFSET,
    .max_value = X_MAX_OFFSET,
    .step = 0.01,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};
static constexpr NumericInputConfig OFFSET_CONFIG_Y {
    .min_value = Y_MIN_OFFSET,
    .max_value = Y_MAX_OFFSET,
    .step = 0.01,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};
static constexpr NumericInputConfig OFFSET_CONFIG_Z {
    .min_value = Z_MIN_OFFSET,
    .max_value = Z_MAX_OFFSET,
    .step = 0.01,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};

MI_OFFSET::MI_OFFSET(const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, float initVal, const NumericInputConfig &config)
    : WiSpin(initVal, config, label, id_icon, enabled, hidden) {}

MI_OFFSET_X::MI_OFFSET_X()
    : MI_OFFSET(_(label), nullptr, is_enabled_t::yes, displayed_tool ? is_hidden_t::no : is_hidden_t::yes, hotend_offset[displayed_tool].x, OFFSET_CONFIG_X) {}

void MI_OFFSET_X::OnClick() {
    hotend_offset[displayed_tool].x = GetVal();
    prusa_toolchanger.save_tool_offsets();
}

MI_OFFSET_Y::MI_OFFSET_Y()
    : MI_OFFSET(_(label), nullptr, is_enabled_t::yes, displayed_tool ? is_hidden_t::no : is_hidden_t::yes, hotend_offset[displayed_tool].y, OFFSET_CONFIG_Y) {}

void MI_OFFSET_Y::OnClick() {
    hotend_offset[displayed_tool].y = GetVal();
    prusa_toolchanger.save_tool_offsets();
}

MI_OFFSET_Z::MI_OFFSET_Z()
    : MI_OFFSET(_(label), nullptr, is_enabled_t::yes, displayed_tool ? is_hidden_t::no : is_hidden_t::yes, hotend_offset[displayed_tool].z, OFFSET_CONFIG_Z) {}

void MI_OFFSET_Z::OnClick() {
    hotend_offset[displayed_tool].z = GetVal();
    prusa_toolchanger.save_tool_offsets();
}

MI_PICKUP_TOOL::MI_PICKUP_TOOL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}

void MI_PICKUP_TOOL::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
    marlin_client::gcode_printf("T%d S1 L0 D0", displayed_tool);
}

MI_FSENSORS_CALIBRATE::MI_FSENSORS_CALIBRATE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FSENSORS_CALIBRATE::click([[maybe_unused]] IWindowMenu &window_menu) {
    if (MsgBoxQuestion(_("Perform filament sensors calibration? This discards previous filament sensors calibration. The extruder will be replaced during calibration"), Responses_YesNo) == Response::No) {
        return;
    }

    marlin_client::test_start_with_data(stmFSensor, static_cast<ToolMask>(1 << displayed_tool));
}

ScreenMenuToolSetup::ScreenMenuToolSetup()
    : detail::ScreenMenuToolSetup(_(labels[displayed_tool])) {
}

I_MI_TOOL::I_MI_TOOL(uint8_t tool_index)
    : IWindowMenuItem(_(tool_name(tool_index)), nullptr, is_enabled_t::yes, prusa_toolchanger.getTool(tool_index).is_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes)
    , tool_index(tool_index) {
}

void I_MI_TOOL::click(IWindowMenu &) {
    displayed_tool = tool_index;
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuToolSetup>);
}

MI_PARK_TOOL::MI_PARK_TOOL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}

void MI_PARK_TOOL::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
    marlin_client::gcode_printf("T%d S1 L0 D0", PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
}

ScreenMenuTools::ScreenMenuTools()
    : detail::ScreenMenuTools(_(label)) {
}

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
