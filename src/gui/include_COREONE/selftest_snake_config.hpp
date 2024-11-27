#pragma once
#include "i18n.h"
#include <utility_extensions.hpp>
#include <printer_selftest.hpp>
#include "selftest_types.hpp"

namespace SelftestSnake {
enum class Tool {
    Tool1 = 0,
    _count,
    _all_tools = _count,
    _last = _count - 1,
    _first = Tool1,
};

// Order matters, snake and will be run in the same order, as well as menu items (with indices) will be
enum class Action {
    Fans,
    YCheck,
    XCheck,
    ZAlign, // also known as z_calib
    Loadcell, // Check loadcell before Z test, because it is used there
    ZCheck,
    Heaters,
    Gears,
    FilamentSensorCalibration,
#if HAS_PHASE_STEPPING()
    PhaseSteppingCalibration,
#endif
    _count,
    _last = _count - 1,
    _first = Fans,
};

template <Action action>
concept SubmenuActionC = false;

constexpr bool has_submenu(Action action) {
    switch (action) {
    default:
        return false;
    }
}

constexpr bool is_multitool_only_action([[maybe_unused]] Action action) {
    return false;
}

constexpr bool requires_toolchanger([[maybe_unused]] Action action) {
    return false;
}

constexpr bool is_singletool_only_action([[maybe_unused]] Action action) {
    return false;
}

consteval auto get_submenu_label(Tool tool, Action action) -> const char * {
    struct ToolText {
        Tool tool;
        Action action;
        const char *label;
    };
    const ToolText tooltexts[] { {} };

    if (auto it = std::ranges::find_if(tooltexts, [&](const auto &elem) {
            return elem.tool == tool && elem.action == action;
        });
        it != std::end(tooltexts)) {
        return it->label;
    } else {
        consteval_assert_false("Unable to find a label for this combination");
        return "";
    }
}

struct MenuItemText {
    Action action;
    const char *label;
};

// could have been done with an array of texts directly, but there would be an order dependancy
inline constexpr MenuItemText blank_item_texts[] {
    { Action::Fans, N_("%d Fan Test") },
        { Action::ZAlign, N_("%d Z Alignment Calibration") },
        { Action::YCheck, N_("%d Y Axis Test") },
        { Action::XCheck, N_("%d X Axis Test") },
        { Action::Loadcell, N_("%d Loadcell Test") },
        { Action::ZCheck, N_("%d Z Axis Test") },
        { Action::Heaters, N_("%d Heater Test") },
        { Action::Gears, N_("%d Gears Calibration") },
        { Action::FilamentSensorCalibration, N_("%d Filament Sensor Calibration") },
#if HAS_PHASE_STEPPING()
        { Action::PhaseSteppingCalibration, N_("%d Phase Stepping Calibration") },
#endif
};

TestResult get_test_result(Action action, Tool tool);
ToolMask get_tool_mask(Tool tool);
uint64_t get_test_mask(Action action);
inline void ask_config([[maybe_unused]] Action action) {}
inline Tool get_last_enabled_tool() { return Tool::Tool1; }
inline Tool get_next_tool(Tool tool) { return tool; }
} // namespace SelftestSnake
