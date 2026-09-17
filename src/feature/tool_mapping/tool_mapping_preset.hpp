/// @file
#pragma once

#include <cstdint>

/// Ways a tool mapping can be automatically (pre)set
/// !!! Used in EEPROM
enum class ToolMappingPreset : uint8_t {
    /// Assigns the tools based on compatibility and whatnot, ignores colors
    smart_without_colors = 0,

    /// Maps gcode tool 1 to virtual tool 1, 2 to 2, ...
    direct = 1,

    /// Assigns nothing, the user has to do the mapping himself
    empty = 2,

    /// Assigns the tools based on compatibility, colors and whatnot
    smart_with_colors = 3,

    _cnt,
};

ToolMappingPreset default_tool_mapping_preset();
