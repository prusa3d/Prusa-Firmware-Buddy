#include "tools_mapping.hpp"
#include <printers.h>
#include <gcode_info.hpp>
#include <module/prusa/tool_mapper.hpp>
#include <mmu2_toolchanger_common.hpp>
#include <print_utils.hpp>
#include <option/has_mmu2.h>
#include <option/has_tool_mapping.h>

#include <option/has_spool_join.h>
#include <bsod/bsod.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

/**
 * @brief Provides helper functions. Expects valid gcode loaded
 *
 */
namespace tools_mapping {
bool is_tool_mapping_possible() {
#if HAS_TOOL_MAPPING()
    #if HAS_MMU2()
    if (!MMU2::mmu2.Enabled()) {
        return false;
    }
    #endif
    return GCodeInfo::getInstance().UsedExtrudersCount() > 1 || (get_num_of_enabled_tools() > 1 && GCodeInfo::getInstance().UsedExtrudersCount() > 0);
#endif
    return false;
}

uint8_t to_physical_tool(uint8_t gcode_tool) {
#if HAS_TOOL_MAPPING()
    if (auto physical_tool = tool_mapper.to_virtual(gcode_tool); physical_tool == ToolMapper::NO_TOOL_MAPPED) {
        return no_tool;
    } else {
        return physical_tool;
    }
#else
    return gcode_tool;
#endif
}

uint8_t to_gcode_tool(uint8_t physical_tool) {
#if HAS_SPOOL_JOIN() && HAS_TOOL_MAPPING()
    return to_gcode_tool_custom(tool_mapper, spool_join, physical_tool);
#elif HAS_TOOL_MAPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    if (auto gcode_tool = tool_mapper.to_gcode(physical_tool); gcode_tool != ToolMapper::NO_TOOL_MAPPED) {
        return gcode_tool;
    } else { // this tool isn't mapped nor joined
        return no_tool;
    }
#else
    return physical_tool;
#endif
}

#if HAS_SPOOL_JOIN() && HAS_TOOL_MAPPING()
uint8_t to_gcode_tool_custom(const ToolMapper &mapper, const SpoolJoin &joiner, uint8_t physical_tool) {
    if (auto gcode_tool = mapper.to_gcode(physical_tool); gcode_tool != ToolMapper::NO_TOOL_MAPPED) {
        return gcode_tool;
    } else if (auto earliest_physical = joiner.get_first_spool_1_from_chain(physical_tool); earliest_physical != physical_tool) {
        auto earliests_gcode_tool = mapper.to_gcode(earliest_physical);
        debug_assert(earliests_gcode_tool != ToolMapper::NO_TOOL_MAPPED); // otherwise invalid spool_join
        return earliests_gcode_tool;
    } else { // this tool isn't mapped nor joined
        return no_tool;
    }
}
#endif
} // namespace tools_mapping
