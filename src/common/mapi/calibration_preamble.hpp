#pragma once

#include <inplace_function.hpp>

#include <option/has_toolchanger.h>

namespace mapi {

struct CalibrationPreamble {
    /// Motion step about to be performed; lets each wizard report its own FSM phase.
    enum class Step : uint8_t {
        moving_away, ///< lowering the bed to the physical bottom
        homing, ///< homing XY
#if HAS_TOOLCHANGER()
        picking_tool, ///< picking a tool (ensure_picked policy only, fired only when no tool is currently picked)
        parking_tool, ///< parking the picked tool (ensure_parked policy only, fired only when a tool is currently picked)
#endif
    };

#if HAS_TOOLCHANGER()
    enum class ToolPolicy : uint8_t {
        keep_as_is, ///< only home XY
        ensure_picked, ///< pick any tool when none is picked
        ensure_parked, ///< park the picked tool (e.g. dock calibration works with an empty head); a tool with an uncalibrated dock parks to the default dock position
    };
    ToolPolicy tool_policy;
#endif

    const stdext::inplace_function<void(Step)> on_step;

    /// Makes subsequent XY moves safe regardless of a possibly unhomed/stale Z:
    /// - lowers the bed
    /// - homes XY (precisely when a toolchange is about to follow)
    /// - picks/parks a tool per tool_policy.
    /// The on_step callback fires before each motion step so wizards can report their FSM phase.
    /// @return false when something fails (caller treats as abort)
    bool run() const;
};

} // namespace mapi
