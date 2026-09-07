/// @file
#include "filament.hpp"

#include <gcode_info.hpp>
#include <marlin_vars.hpp>

FilamentType FilamentType::for_tool(std::variant<VirtualToolIndex, NoTool> tool) {
    return match(
        tool, //
        [](VirtualToolIndex virtual_tool) { return config_store().get_filament_type(virtual_tool); }, //
        [](NoTool) { return none; } //
    );
}

FilamentType FilamentType::for_tool_heuristic(std::variant<VirtualToolIndex, NoTool> tool) {
    if (auto filament = for_tool(tool)) {
        return filament;
    }

    if (!std::holds_alternative<VirtualToolIndex>(tool)) {
        return FilamentType::none;
    }
    const auto virtual_tool = std::get<VirtualToolIndex>(tool);

    // Try to get data from the GCodeInfo if we're printing
    if (marlin_vars().print_state != marlin_server::State::Idle) {
        for (auto gcode_tool : GcodeToolIndex::all()) {
            if (!stdext::holds_value(gcode_tool.to_virtual(), virtual_tool)) {
                continue;
            }

            const auto &filament_name = GCodeInfo::getInstance().get_extruder_info(gcode_tool).filament_name;
            if (auto filament = from_name(filament_name)) {
                return filament;
            }
        }
    }

    // Last resort - check previously loaded filament
    if (auto filament = config_store().get_previous_filament_type(virtual_tool)) {
        return filament;
    }

    return NoFilamentType {};
}

FilamentType FilamentType::for_current_tool_heuristic() {
    return for_tool_heuristic(VirtualToolIndex::currently_selected());
}
