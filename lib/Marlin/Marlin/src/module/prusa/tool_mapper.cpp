#include "inc/MarlinConfig.h"
#include <cstddef>
#include <iterator>

#if ENABLED(PRUSA_TOOL_MAPPING)

    #include "module/prusa/tool_mapper.hpp"
    #include "module/prusa/toolchanger.h"

ToolMapper tool_mapper;

ToolMapper::ToolMapper() {
    reset();
}

bool ToolMapper::set_mapping(uint8_t logical, uint8_t physical) {
    // physical tool is enabled and valid
    if (physical >= EXTRUDERS || !prusa_toolchanger.is_tool_enabled(physical)) {
        return false;
    }

    // check that logical tool is valid as well
    if (logical >= EXTRUDERS || logical == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return false;
    }

    // if this physical tool is already mapped to some logical tool, remove this assignment
    uint8_t previous_logical = to_logical(physical);
    if (previous_logical != NO_TOOL_MAPPED) {
        logical_to_physical[previous_logical] = NO_TOOL_MAPPED;
    }

    // do the mapping
    logical_to_physical[logical] = physical;
    return true;
}

void ToolMapper::set_enable(bool enable) {
    this->enabled = enable;
}

uint8_t ToolMapper::to_physical(uint8_t logical, bool ignore_enabled) {
    if ((ignore_enabled || enabled) && logical < std::size(logical_to_physical))
        return logical_to_physical[logical];
    else
        return logical; // no maping
}

uint8_t ToolMapper::to_logical(uint8_t physical) {
    for (size_t i = 0; i < std::size(logical_to_physical); i++) {
        if (logical_to_physical[i] == physical) {
            return i;
        }
    }
    return NO_TOOL_MAPPED;
}

void ToolMapper::reset() {
    for (size_t i = 0; i < std::size(logical_to_physical); i++) {
        logical_to_physical[i] = i;
    }
    enabled = false;
}

#endif
