/**
 * @file screen_menu_tools.cpp
 */

#include "screen_menu_tools.hpp"
#include "ScreenHandler.hpp"
#include "png_resources.hpp"

#include "module/motion.h"
#include "module/prusa/toolchanger.h"
#include "marlin_client.hpp"

#include <limits>

static int displayed_tool = 0;

MI_TOOL_NOZZLE_DIAMETER::MI_TOOL_NOZZLE_DIAMETER()
    : MI_NOZZLE_DIAMETER(displayed_tool, is_hidden_t::no) {
}

static constexpr SpinConfig_t<float> POSITION_CONFIG({ 0, 500, 0.1 }, "mm");

MI_POSITION::MI_POSITION(string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, float initVal)
    : WiSpinFlt(initVal, POSITION_CONFIG, label, nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_POSITION::OnClick() {
    set_pos(GetVal());
    prusa_toolchanger.save_tool_info();
}

MI_KENNEL_POSITION_X::MI_KENNEL_POSITION_X()
    : MI_POSITION(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, prusa_toolchanger.get_tool_info(prusa_toolchanger.getTool(displayed_tool)).kennel_x) {}

void MI_KENNEL_POSITION_X::set_pos(const float pos) {
    const buddy::puppies::Dwarf &dwarf = prusa_toolchanger.getTool(displayed_tool);
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(dwarf);
    info.kennel_x = pos;
    prusa_toolchanger.set_tool_info(dwarf, info);
}

MI_KENNEL_POSITION_Y::MI_KENNEL_POSITION_Y()
    : MI_POSITION(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, prusa_toolchanger.get_tool_info(prusa_toolchanger.getTool(displayed_tool)).kennel_y) {}

void MI_KENNEL_POSITION_Y::set_pos(const float pos) {
    const buddy::puppies::Dwarf &dwarf = prusa_toolchanger.getTool(displayed_tool);
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(dwarf);
    info.kennel_y = pos;
    prusa_toolchanger.set_tool_info(dwarf, info);
}

static constexpr SpinConfig_t<float> OFFSET_CONFIG({ -10, 10, 0.01 }, "mm");

MI_OFFSET::MI_OFFSET(string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, float initVal)
    : WiSpinFlt(initVal, OFFSET_CONFIG, label, nullptr, is_enabled_t::yes, is_hidden_t::no) {}

MI_OFFSET_X::MI_OFFSET_X()
    : MI_OFFSET(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, hotend_offset[displayed_tool].x) {}

void MI_OFFSET_X::OnClick() {
    hotend_offset[displayed_tool].x = GetVal();
    prusa_toolchanger.save_tool_offsets();
}

MI_OFFSET_Y::MI_OFFSET_Y()
    : MI_OFFSET(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, hotend_offset[displayed_tool].y) {}

void MI_OFFSET_Y::OnClick() {
    hotend_offset[displayed_tool].y = GetVal();
    prusa_toolchanger.save_tool_offsets();
}

MI_OFFSET_Z::MI_OFFSET_Z()
    : MI_OFFSET(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, hotend_offset[displayed_tool].z) {}

void MI_OFFSET_Z::OnClick() {
    hotend_offset[displayed_tool].z = GetVal();
    prusa_toolchanger.save_tool_offsets();
}

MI_PICKUP_TOOL::MI_PICKUP_TOOL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}

void MI_PICKUP_TOOL::click(IWindowMenu &window_menu) {
    marlin_gcode_printf("T%d S", displayed_tool);
}

MI_KENNEL_CALIBRATE::MI_KENNEL_CALIBRATE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_KENNEL_CALIBRATE::click(IWindowMenu &window_menu) {
    marlin_test_start_for_tools(stmKennels, 1 << displayed_tool);
}

MI_FSENSORS_CALIBRATE::MI_FSENSORS_CALIBRATE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FSENSORS_CALIBRATE::click(IWindowMenu &window_menu) {
    if (MsgBoxQuestion(_("Perform filament sensors calibration? This discards previous filament sensors calibration. The extruder will be replaced during calibration"), Responses_YesNo) == Response::No) {
        return;
    }

    marlin_test_start_for_tools(stmFSensor, 1 << displayed_tool);
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

void MI_PARK_TOOL::click(IWindowMenu &window_menu) {
    marlin_gcode_printf("T%d S", PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
}

ScreenMenuTools::ScreenMenuTools()
    : detail::ScreenMenuTools(_(label)) {
}
