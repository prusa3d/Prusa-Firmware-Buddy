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

MI_TOOL_NOZZLE_DIAMETER::MI_TOOL_NOZZLE_DIAMETER()
    : MI_NOZZLE_DIAMETER(displayed_tool, is_hidden_t::no) {
}

static constexpr SpinConfig_t<float> POSITION_CONFIG({ 0, 500, 0.1 }, "mm");

MI_POSITION::MI_POSITION(string_view_utf8 label, [[maybe_unused]] const img::Resource *id_icon, [[maybe_unused]] is_enabled_t enabled, [[maybe_unused]] is_hidden_t hidden, float initVal)
    : WiSpinFlt(initVal, POSITION_CONFIG, label, nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_POSITION::OnClick() {
    set_pos(GetVal());
    prusa_toolchanger.save_tool_info();
}

MI_DOCK_POSITION_X::MI_DOCK_POSITION_X()
    : MI_POSITION(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, prusa_toolchanger.get_tool_info(prusa_toolchanger.getTool(displayed_tool)).dock_x) {}

void MI_DOCK_POSITION_X::set_pos(const float pos) {
    const buddy::puppies::Dwarf &dwarf = prusa_toolchanger.getTool(displayed_tool);
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(dwarf);
    info.dock_x = pos;
    prusa_toolchanger.set_tool_info(dwarf, info);
}

MI_DOCK_POSITION_Y::MI_DOCK_POSITION_Y()
    : MI_POSITION(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, prusa_toolchanger.get_tool_info(prusa_toolchanger.getTool(displayed_tool)).dock_y) {}

void MI_DOCK_POSITION_Y::set_pos(const float pos) {
    const buddy::puppies::Dwarf &dwarf = prusa_toolchanger.getTool(displayed_tool);
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(dwarf);
    info.dock_y = pos;
    prusa_toolchanger.set_tool_info(dwarf, info);
}

static constexpr SpinConfig_t<float> OFFSET_CONFIG_X({ X_MIN_OFFSET, X_MAX_OFFSET, 0.01 }, "mm");
static constexpr SpinConfig_t<float> OFFSET_CONFIG_Y({ Y_MIN_OFFSET, Y_MAX_OFFSET, 0.01 }, "mm");
static constexpr SpinConfig_t<float> OFFSET_CONFIG_Z({ Z_MIN_OFFSET, Z_MAX_OFFSET, 0.01 }, "mm");

MI_OFFSET::MI_OFFSET(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, float initVal, const SpinConfig_t<float> &config)
    : WiSpinFlt(initVal, config, label, id_icon, enabled, hidden) {}

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
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}

void MI_PICKUP_TOOL::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
    marlin_client::gcode_printf("T%d S1 L0 D0", displayed_tool);
}

MI_DOCK_CALIBRATE::MI_DOCK_CALIBRATE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_DOCK_CALIBRATE::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::test_start_for_tools(stmDocks, 1 << displayed_tool);
}

MI_FSENSORS_CALIBRATE::MI_FSENSORS_CALIBRATE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FSENSORS_CALIBRATE::click([[maybe_unused]] IWindowMenu &window_menu) {
    if (MsgBoxQuestion(_("Perform filament sensors calibration? This discards previous filament sensors calibration. The extruder will be replaced during calibration"), Responses_YesNo) == Response::No) {
        return;
    }

    marlin_client::test_start_for_tools(stmFSensor, 1 << displayed_tool);
}

ScreenMenuToolSetup::ScreenMenuToolSetup()
    : detail::ScreenMenuToolSetup(_(labels[displayed_tool])) {
}

I_MI_TOOL::I_MI_TOOL(const char *const label, int index)
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.getTool(index).is_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void I_MI_TOOL::do_click(int index) {
    displayed_tool = index;
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuToolSetup>);
}

MI_PARK_TOOL::MI_PARK_TOOL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
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
