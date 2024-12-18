#include "inc/MarlinConfig.h"
#include <cstddef>
#include <iterator>
#include <mutex>

#if ENABLED(PRUSA_TOOL_MAPPING)

    #include "module/prusa/tool_mapper.hpp"
    #include "mmu2_toolchanger_common.hpp"

// This value is important only for XL, for MMU it should just be something bigger than 5 (num of slots)
uint8_t get_invalid_tool_number() {
    #if HAS_TOOLCHANGER()
    return PrusaToolChanger::MARLIN_NO_TOOL_PICKED;
    #elif HAS_MMU2()
    return 6; // MMU has 5 slots
    #endif
}

ToolMapper tool_mapper;

ToolMapper::ToolMapper() {
    reset();
}

ToolMapper &ToolMapper::operator=(const ToolMapper &other) {
    std::scoped_lock lock(mutex, other.mutex);
    this->enabled = other.enabled;
    for (size_t i = 0; i < std::size(gcode_to_physical); i++) {
        this->gcode_to_physical[i] = other.gcode_to_physical[i];
    }
    return *this;
}

bool ToolMapper::set_mapping(uint8_t logical, uint8_t physical) {
    std::unique_lock lock(mutex);
    // physical tool is enabled and valid
    if (physical >= EXTRUDERS || !is_tool_enabled(physical)) {
        return false;
    }

    // check that logical tool is valid as well
    if (logical >= EXTRUDERS || logical == get_invalid_tool_number()) {
        return false;
    }

    // if this physical tool is already mapped to some logical tool, remove this assignment
    uint8_t previous_logical = to_gcode_unlocked(physical);
    if (previous_logical != NO_TOOL_MAPPED) {
        gcode_to_physical[previous_logical] = NO_TOOL_MAPPED;
    }

    // do the mapping
    gcode_to_physical[logical] = physical;
    return true;
}

bool ToolMapper::set_unassigned(uint8_t logical) {
    std::unique_lock lock(mutex);
    return set_unassigned_unlocked(logical);
}

bool ToolMapper::set_unassigned_unlocked(uint8_t logical) {
    // check that logical tool is valid
    if (logical >= EXTRUDERS || logical == get_invalid_tool_number()) {
        return false;
    }

    gcode_to_physical[logical] = NO_TOOL_MAPPED;
    return true;
}

void ToolMapper::set_enable(bool enable) {
    std::unique_lock lock(mutex);
    this->enabled = enable;
}

uint8_t ToolMapper::to_physical(uint8_t logical, bool ignore_enabled) const {
    std::unique_lock lock(mutex);
    if ((ignore_enabled || enabled) && logical < std::size(gcode_to_physical)) {
        return gcode_to_physical[logical];
    } else {
        return logical; // no maping
    }
}
uint8_t ToolMapper::to_gcode(uint8_t physical) const {
    std::unique_lock lock(mutex);
    return to_gcode_unlocked(physical);
}

uint8_t ToolMapper::to_gcode_unlocked(uint8_t physical) const {
    for (size_t i = 0; i < std::size(gcode_to_physical); i++) {
        if (gcode_to_physical[i] == physical) {
            return i;
        }
    }
    return NO_TOOL_MAPPED;
}

void ToolMapper::reset() {
    std::unique_lock lock(mutex);
    for (size_t i = 0; i < std::size(gcode_to_physical); i++) {
        gcode_to_physical[i] = i;
    }
    enabled = false;
}

void ToolMapper::set_all_unassigned() {
    std::unique_lock lock(mutex);
    EXTRUDER_LOOP() {
        set_unassigned_unlocked(e);
    }
}

void ToolMapper::serialize(serialized_state_t &to) {
    // NOTE: We do not lock here now, as it is not possible other thread would be modifying
    // the objekt at this point (they do that before starting the print). If this ever changes
    // we should rethink this, this is called from default task, not ISR, so it might be ok to lock.
    to.enabled = enabled;
    EXTRUDER_LOOP() {
        to.gcode_to_physical[e] = gcode_to_physical[e];
    }
}

void ToolMapper::deserialize(serialized_state_t &from) {
    std::unique_lock lock(mutex);
    enabled = from.enabled;
    EXTRUDER_LOOP() {
        gcode_to_physical[e] = from.gcode_to_physical[e];
    }
}

#endif
