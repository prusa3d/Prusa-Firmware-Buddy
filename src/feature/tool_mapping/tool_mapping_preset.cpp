#include "tool_mapping_preset.hpp"

#include <utility>

#include <config_store/store_instance.hpp>

ToolMappingPreset default_tool_mapping_preset() {
    // The stored value can be out of range (different FW version or defaulted)
    const auto stored = config_store().tool_mapping_preset.get();
    if (stored < std::to_underlying(ToolMappingPreset::_cnt)) {
        return static_cast<ToolMappingPreset>(stored);
    }

    return ToolMappingPreset::smart_without_colors;
}
