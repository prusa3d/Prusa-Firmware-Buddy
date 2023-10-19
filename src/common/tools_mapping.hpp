#pragma once
#include <limits>
#include <stdint.h>
#include <functional>
#include <module/prusa/tool_mapper.hpp>
#include <module/prusa/spool_join.hpp>

namespace tools_mapping {

inline constexpr uint8_t no_tool { std::numeric_limits<uint8_t>::max() };

/**
 * @brief Returns true if tools_mapping functionality is possible
 *
 */
bool is_tool_mapping_possible();

/**
 * @brief Returns the earliest spool_1 (mapping) of physical_tool to the given gcode_tool. Can return no_tool
 *
 * @param gcode_tool
 */
uint8_t to_physical_tool(uint8_t gcode_tool);

/**
 * @brief Returns the gcode_tool that is printed by given physical_tool
 *
 * @param physical_tool
 */
uint8_t to_gcode_tool(uint8_t physical_tool);

#if ENABLED(PRUSA_SPOOL_JOIN) && ENABLED(PRUSA_TOOL_MAPPING)
/**
 * @brief Returns the gcode_tool that is printed by given physical_tool, with the given mapper/joiner configuration
 *
 * @param mapper
 * @param joiner
 * @param physical_tool
 */
uint8_t to_gcode_tool_custom(const ToolMapper &mapper, const SpoolJoin &joiner, uint8_t physical_tool);
#endif

/**
 * @brief Executes executable on a whole spool join chain
 *
 * @param physical_tool a tool anywhere in a chain
 * @param executable
 */
void execute_on_whole_chain(uint8_t physical_tool, std::function<void(uint8_t)> executable);
} // namespace tools_mapping
