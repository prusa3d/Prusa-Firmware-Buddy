#include "tools_mapping.hpp"
#include <printers.h>
#include <gcode_info.hpp>
#include <module/prusa/tool_mapper.hpp>
#include <module/prusa/spool_join.hpp>
#include <mmu2_toolchanger_common.hpp>

/**
 * @brief Provides helper functions. Expects valid gcode loaded
 *
 */
namespace tools_mapping {
bool is_tool_mapping_possible() {
#if HAS_TOOLCHANGER() || HAS_MMU2()
    return GCodeInfo::getInstance().UsedExtrudersCount() > 1 || (get_num_of_enabled_tools() > 1 && GCodeInfo::getInstance().UsedExtrudersCount() > 0);
#endif
    return false;
}

uint8_t to_physical_tool(uint8_t gcode_tool) {
#if ENABLED(PRUSA_TOOL_MAPPING)
    if (auto physical_tool = tool_mapper.to_physical(gcode_tool); physical_tool == ToolMapper::NO_TOOL_MAPPED) {
        return no_tool;
    } else {
        return physical_tool;
    }
#else
    return gcode_tool;
#endif
}

uint8_t to_gcode_tool(uint8_t physical_tool) {
#if ENABLED(PRUSA_SPOOL_JOIN) && ENABLED(PRUSA_TOOL_MAPPING)
    return to_gcode_tool_custom(tool_mapper, spool_join, physical_tool);
#elif ENABLED(PRUSA_TOOL_MAPPING)
    if (auto gcode_tool = tool_mapper.to_gcode(physical_tool); gcode_tool != ToolMapper::NO_TOOL_MAPPED) {
        return gcode_tool;
    } else { // this tool isn't mapped nor joined
        return no_tool;
    }
#else
    return physical_tool;
#endif
}

#if ENABLED(PRUSA_SPOOL_JOIN) && ENABLED(PRUSA_TOOL_MAPPING)
uint8_t to_gcode_tool_custom(const ToolMapper &mapper, const SpoolJoin &joiner, uint8_t physical_tool) {
    if (auto gcode_tool = mapper.to_gcode(physical_tool); gcode_tool != ToolMapper::NO_TOOL_MAPPED) {
        return gcode_tool;
    } else if (auto earliest_physical = joiner.get_first_spool_1_from_chain(physical_tool); earliest_physical != physical_tool) {
        auto earliests_gcode_tool = mapper.to_gcode(earliest_physical);
        assert(earliests_gcode_tool != ToolMapper::NO_TOOL_MAPPED); // otherwise invalid spool_join
        return earliests_gcode_tool;
    } else { // this tool isn't mapped nor joined
        return no_tool;
    }
}
#endif

void execute_on_whole_chain(uint8_t physical_tool, std::function<void(uint8_t)> executable) {
#if ENABLED(PRUSA_SPOOL_JOIN)
    executable(spool_join.get_first_spool_1_from_chain(physical_tool));

    auto followup_spool = spool_join.get_spool_2(physical_tool);
    while (followup_spool.has_value()) {
        executable(followup_spool.value());
        followup_spool = spool_join.get_spool_2(followup_spool.value());
    }
#else
    executable(physical_tool);
#endif
}
} // namespace tools_mapping
