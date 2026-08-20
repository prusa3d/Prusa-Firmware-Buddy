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

        // Note: the number was chosen arbitrarily, in practice anything above 1 cm would possibly do
        if (!park(ParkingPosition { .z = ParkingPosition::AtLeast { .absolute = 50 } })) {
            return false;
        }
    }

    // Home XY before a possible toolchange. A toolchange requires precise homing
    const auto ensure_xy_homed = [&](AxisHomeLevel level) -> bool {
        if (axes_home_level.is_homed({ X_AXIS, Y_AXIS }, level)) {
            return true;
        }
        on_step(Step::homing);
        // z_raise=0: Z is already safely at the bottom from the move above
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
        // Z is already safe at the bottom: skip the Z lift and don't return Z anywhere
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
            // Z is already safe at the bottom: skip the Z lift and don't return Z anywhere
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
