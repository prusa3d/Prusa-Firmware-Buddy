#include "screen_toolhead_settings_dock.hpp"

using namespace screen_toolhead_settings;

static constexpr NumericInputConfig dock_position_config {
    .max_value = 500,
    .step = 0.1,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};

// * MI_DOCK_X
MI_DOCK_X::MI_DOCK_X(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, dock_position_config, _("Dock X")) {
    update();
}

float MI_DOCK_X::read_value_impl(ToolheadIndex ix) {
    return prusa_toolchanger.get_tool_info(prusa_toolchanger.getTool(ix)).dock_x;
}

void MI_DOCK_X::store_value_impl(ToolheadIndex ix, float set) {
    const buddy::puppies::Dwarf &dwarf = prusa_toolchanger.getTool(ix);
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(dwarf);
    info.dock_x = set;
    prusa_toolchanger.set_tool_info(dwarf, info);
    prusa_toolchanger.save_tool_info();
}

// * MI_DOCK_Y
MI_DOCK_Y::MI_DOCK_Y(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, dock_position_config, _("Dock Y")) {
    update();
}

float MI_DOCK_Y::read_value_impl(ToolheadIndex ix) {
    return prusa_toolchanger.get_tool_info(prusa_toolchanger.getTool(ix)).dock_y;
}

void MI_DOCK_Y::store_value_impl(ToolheadIndex ix, float set) {
    const buddy::puppies::Dwarf &dwarf = prusa_toolchanger.getTool(ix);
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(dwarf);
    info.dock_y = set;
    prusa_toolchanger.set_tool_info(dwarf, info);
    prusa_toolchanger.save_tool_info();
}

// * MI_DOCK_CALIBRATE
MI_DOCK_CALIBRATE::MI_DOCK_CALIBRATE(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC(toolhead, _("Calibrate Dock Position")) {
}

void MI_DOCK_CALIBRATE::click(IWindowMenu &) {
    marlin_client::test_start_with_data(stmDocks, static_cast<ToolMask>(1 << std::get<ToolheadIndex>(toolhead())));
}

// * ScreenToolheadDetailDock
ScreenToolheadDetailDock::ScreenToolheadDetailDock(Toolhead toolhead)
    : ScreenMenu(_("DOCK CONFIGURATION"))
    , toolhead(toolhead) //
{
    menu_set_toolhead(container, toolhead);
}
