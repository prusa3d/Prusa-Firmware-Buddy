#include "../../inc/MarlinConfig.h"
#include "screen_toolhead_settings_dock.hpp"
#include <option/has_dwarf.h>
#include <option/has_indx.h>
#if HAS_INDX()
    #include <config_store/store_instance.hpp>
    #include <window_msgbox.hpp>
    #include <marlin_client.hpp>
    #include <gui/dialogs/window_dlg_wait.hpp>
    #include <utils/variant_utils.hpp>
#endif

using namespace screen_toolhead_settings;

static constexpr NumericInputConfig dock_position_config_x {
    .min_value = X_MIN_POS,
    .max_value = X_MAX_POS,
    .step = 0.1f,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};

static constexpr NumericInputConfig dock_position_config_y {
    .min_value = Y_MIN_POS,
    .max_value = Y_MAX_POS,
    .step = 0.1f,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};

// * MI_DOCK_X
MI_DOCK_X::MI_DOCK_X(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, dock_position_config_x, _("Dock X")) {
    update();
}

float MI_DOCK_X::read_value_impl(PhysicalToolIndex ix) {
    return prusa_toolchanger.get_tool_info(ix).dock_x;
}

void MI_DOCK_X::store_value_impl(PhysicalToolIndex ix, float set) {
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(ix);
    info.dock_x = set;
    prusa_toolchanger.set_tool_info(ix, info);
    prusa_toolchanger.save_tool_info();
}

// * MI_DOCK_Y
MI_DOCK_Y::MI_DOCK_Y(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_SPIN(toolhead, 0, dock_position_config_y, _("Dock Y")) {
    update();
}

float MI_DOCK_Y::read_value_impl(PhysicalToolIndex ix) {
    return prusa_toolchanger.get_tool_info(ix).dock_y;
}

void MI_DOCK_Y::store_value_impl(PhysicalToolIndex ix, float set) {
    PrusaToolInfo info = prusa_toolchanger.get_tool_info(ix);
    info.dock_y = set;
    prusa_toolchanger.set_tool_info(ix, info);
    prusa_toolchanger.save_tool_info();
}

#if HAS_SELFTEST() && !HAS_INDX()
// * MI_DOCK_CALIBRATE
MI_DOCK_CALIBRATE::MI_DOCK_CALIBRATE(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_BASE(toolhead, _("Calibrate Dock Position")) {
}

void MI_DOCK_CALIBRATE::click([[maybe_unused]] IWindowMenu &menu) {
    marlin_client::test_start_with_data(stmDocks, std::get<PhysicalToolIndex>(toolhead()));
}
#endif

#if HAS_INDX()
// * MI_DOCK_INVALIDATE_CALIBRATION
MI_DOCK_INVALIDATE_CALIBRATION::MI_DOCK_INVALIDATE_CALIBRATION(Toolhead toolhead)
    : MI_TOOLHEAD_SPECIFIC_BASE(toolhead, _("Invalidate Calibration")) {
}

void MI_DOCK_INVALIDATE_CALIBRATION::click([[maybe_unused]] IWindowMenu &menu) {
    if (MsgBoxWarning(_("This will invalidate the calibration for this tool. The tool will not be available for printing until its dock is calibrated again. Proceed?"), { Response::Continue, Response::Back, Response::_none, Response::_none }) != Response::Continue) {
        return;
    }

    const auto dock_index = std::get<PhysicalToolIndex>(toolhead());

    if (stdext::holds_value(PhysicalToolIndex::currently_selected(), dock_index)) {
        // Park first — once the dock is uncalibrated, we wouldn't know where exactly to park.
        marlin_client::gcode("P0");
        window_dlg_wait_t::wait_for_gcodes_to_finish();

        // If parking failed for any reason, keep the dock calibrated.
        if (stdext::holds_value(PhysicalToolIndex::currently_selected(), dock_index)) {
            MsgBoxError(_("Could not park the tool. Calibration preserved."), Responses_Ok);
            return;
        }
    }

    auto calibrated_mask = config_store().indx_dock_calibrated_mask.get();
    calibrated_mask.reset(dock_index.to_raw());
    config_store().indx_dock_calibrated_mask.set(calibrated_mask);
}
#endif

// * ScreenToolheadDetailDock
ScreenToolheadDetailDock::ScreenToolheadDetailDock(Toolhead toolhead)
    : ScreenMenu(_("DOCK POSITION"))
    , toolhead(toolhead) //
{
    menu_set_toolhead(container, toolhead);
}
