#include "module/prusa/tool_mapper.hpp"

#include <utils/variant_utils.hpp>

ToolMapper tool_mapper;

ToolMapper &ToolMapper::operator=(const GcodeToVirtualArray &mapping) {
    std::scoped_lock lock(mutex);
    this->enabled = true;
    this->gcode_to_virtual = mapping;
    return *this;
}

ToolMapper &ToolMapper::operator=(const ToolMapper &other) {
    std::scoped_lock lock(mutex, other.mutex);
    this->enabled = other.enabled;
    this->gcode_to_virtual = other.gcode_to_virtual;
    return *this;
}

bool ToolMapper::set_mapping(GcodeToolIndex gcode_tool, VirtualToolIndex virtual_tool) {
    std::unique_lock lock(mutex);
    if (!virtual_tool.is_enabled()) {
        return false;
    }

    // if this virtual tool is already mapped to some gcode tool, remove this assignment
    match(
        to_gcode_unlocked(virtual_tool),
        [this](GcodeToolIndex gcode_tool) {
            gcode_to_virtual[gcode_tool] = ToolNotMapped {};
        },
        [](ToolNotMapped) {});

    // do the mapping
    gcode_to_virtual[gcode_tool] = virtual_tool;
    return true;
}

ToolMapper::ToVirtualResult ToolMapper::to_virtual(GcodeToolIndex gcode_tool, bool ignore_enabled) const {
    std::unique_lock lock(mutex);
    if (ignore_enabled || enabled) {
        return gcode_to_virtual[gcode_tool];
    } else {
        return VirtualToolIndex::from_raw(gcode_tool.to_raw()); // no mapping
    }
}

ToolMapper::ToGCodeResult ToolMapper::to_gcode_unlocked(VirtualToolIndex virtual_tool) const {
    for (auto gcode_tool : GcodeToolIndex::all()) {
        if (stdext::holds_value(gcode_to_virtual[gcode_tool], virtual_tool)) {
            return gcode_tool;
        }
    }
    return ToolNotMapped {};
}

void ToolMapper::serialize(serialized_state_t &to) {
    // NOTE: We do not lock here now, as it is not possible other thread would be modifying
    // the objekt at this point (they do that before starting the print). If this ever changes
    // we should rethink this, this is called from default task, not ISR, so it might be ok to lock.
    to.enabled = enabled;
    for (auto gcode_tool : GcodeToolIndex::all()) {
        to.gcode_to_virtual[gcode_tool.to_raw()] = match(
            gcode_to_virtual[gcode_tool],
            [](VirtualToolIndex virtual_tool) { return virtual_tool.to_raw(); },
            [](ToolNotMapped) { return NO_TOOL_MAPPED; });
    }
}

void ToolMapper::deserialize(serialized_state_t &from) {
    std::unique_lock lock(mutex);
    enabled = from.enabled;
    for (auto gcode_tool : GcodeToolIndex::all()) {
        const auto virtual_raw = from.gcode_to_virtual[gcode_tool.to_raw()];
        gcode_to_virtual[gcode_tool] = (virtual_raw == NO_TOOL_MAPPED) ? ToVirtualResult { ToolNotMapped {} } : VirtualToolIndex::from_raw(virtual_raw);
    }
}
