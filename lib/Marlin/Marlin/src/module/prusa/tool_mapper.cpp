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

bool ToolMapper::set_unassigned(uint8_t logical) {
    // check that logical tool is valid
    if (logical >= EXTRUDERS || logical == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return false;
    }

    logical_to_physical[logical] = NO_TOOL_MAPPED;
    return true;
}

void ToolMapper::set_enable(bool enable) {
    this->enabled = enable;
}

uint8_t ToolMapper::to_physical(uint8_t logical, bool ignore_enabled) const {
    if ((ignore_enabled || enabled) && logical < std::size(logical_to_physical))
        return logical_to_physical[logical];
    else
        return logical; // no maping
}

uint8_t ToolMapper::to_logical(uint8_t physical) const {
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

void ToolMapper::set_all_unassigned() {
    for (auto &elem : logical_to_physical) {
        elem = NO_TOOL_MAPPED;
    }
}

void ToolMapper::serialize(serialized_state_t &to) {
    to.enabled = enabled;
    EXTRUDER_LOOP() {
        to.logical_to_physical[e] = logical_to_physical[e];
    }
}

void ToolMapper::deserialize(serialized_state_t &from) {
    enabled = from.enabled;
    EXTRUDER_LOOP() {
        logical_to_physical[e] = from.logical_to_physical[e];
    }
}

#endif
