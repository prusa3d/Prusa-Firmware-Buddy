#include "calibration_preamble.hpp"

#include <variant>

#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/motion.h>
#include <mapi/parking.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

namespace mapi {

bool CalibrationPreamble::run() const {
    {
        // Make sure we have enough clearance above the bed
        on_step(Step::moving_away);
        if (!park(moving_away_position)) {
            return false;
        }
    }

    // Home XY before a possible toolchange. A toolchange requires precise homing
    const auto ensure_xy_homed = [&](AxisHomeLevel level) -> bool {
        if (axes_home_level.is_homed({ X_AXIS, Y_AXIS }, level)) {
            return true;
        }
        on_step(Step::homing);
        // z_raise=0: the bed is already lowered
        return GcodeSuite::G28_no_parser(true, true, false, { .z_raise = 0, .precise = level == AxisHomeLevel::full });
    };

#if HAS_TOOLCHANGER()
    const auto current_tool = PhysicalToolIndex::currently_selected_opt();

    switch (tool_policy) {

    case ToolPolicy::keep_as_is:
        break;

    case ToolPolicy::ensure_picked: {
        if (current_tool.has_value()) {
            // Already picked
            break;
        }

        if (!ensure_xy_homed(AxisHomeLevel::full)) {
            return false;
        }

        on_step(Step::picking_tool);
        // The bed is already lowered: skip the Z lift and don't return Z anywhere
        if (!prusa_toolchanger.pick_any_tool(tool_return_t::no_return, {}, tool_change_lift_t::no_lift, false)) {
            return false;
        }
        break;
    }

    case ToolPolicy::ensure_parked: {
        if (!current_tool.has_value()) {
            // Already parked
            break;
        }

        if (!ensure_xy_homed(AxisHomeLevel::full)) {
            return false;
        }

        on_step(Step::parking_tool);
        if (current_tool->is_enabled()) {
            // The bed is already lowered: skip the Z lift and don't return Z anywhere
            if (!prusa_toolchanger.tool_change(NoTool {}, tool_return_t::no_return, {}, tool_change_lift_t::no_lift, false)) {
                return false;
            }

        } else {
    #if HAS_INDX()
            // Dock not calibrated yet: park to the default dock position,
            // with a bump check first to make sure the dock is empty
            if (!prusa_toolchanger.manual_tool_park(*current_tool)) {
                return false;
            }
    #elif PRINTER_IS_PRUSA_XL()
            // Shouldn't be possible
            bsod_unreachable();
    #else
        #error
    #endif
        }
        break;
    }
    }
#endif

    return ensure_xy_homed(AxisHomeLevel::imprecise);
}

} // namespace mapi
