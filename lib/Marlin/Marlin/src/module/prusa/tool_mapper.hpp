#pragma once

#include "inc/MarlinConfig.h"
#include <limits>

#if ENABLED(PRUSA_TOOL_MAPPING)

/**
 * @brief Tool mapper allows user to assign tools in GUI/GCODE to other tools, depending on where he has filament he wants to use
 *
 * @note  Logical tool - Tool that gcode refers to (eg. T1)
 *        Physical tool - Actual physical tool on printer
 *        Mapping of logical tool 1 to physical tool 3 means that whenever gcode requests tool 1, printer will actually use tool 3
 */
class ToolMapper {
public:
    ToolMapper();

    /// Create new mapping of tool
    bool set_mapping(uint8_t logical, uint8_t physical);

    /// Enable or disable all tool mappings
    void set_enable(bool enable);

    /// true when mapping is enabled
    inline bool is_enabled() { return enabled; }

    /// Convert logical tool to physical
    /// note: might return NO_TOOL_MAPPED, so check for this value
    uint8_t to_physical(uint8_t logical, bool ignore_enabled = false);

    /// Convert physical tool to logical
    uint8_t to_logical(uint8_t physical);

    /// Reset all tool mapping
    void reset();

    // This is special tool identifier, that says that this tool is not mapped to any tool, and is threfore disabled by tool mapping
    static constexpr auto NO_TOOL_MAPPED = std::numeric_limits<uint8_t>::max();

private:
    bool enabled;
    uint8_t logical_to_physical[EXTRUDERS];
};

extern ToolMapper tool_mapper;

#endif
