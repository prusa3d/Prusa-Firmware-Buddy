#pragma once

#include <freertos/mutex.hpp>
#include <utils/overloaded_visitor.hpp>
#include <limits>
#include <mutex>
#include <tool_index.hpp>
#include <utils/array_extensions.hpp>

#include <option/has_tool_mapping.h>
static_assert(HAS_TOOL_MAPPING());

/**
 * @brief Tool mapper allows user to assign tools in GUI/GCODE to virtual tools, depending on where he has filament he wants to use
 *
 * @note  Gcode tool - Tool that gcode refers to (eg. T1)
 *        Virtual tool - Abstraction above physical tool to enable filament multiplexing
 *        Mapping of gcode tool 1 to virtual tool 3 means that whenever gcode requests tool 1, printer will actually use tool 3
 */
class ToolMapper {
public:
    using ToVirtualResult = std::variant<VirtualToolIndex, ToolNotMapped>;
    using ToGCodeResult = std::variant<GcodeToolIndex, ToolNotMapped>;

    using GcodeToVirtualArray = StrongIndexArray<ToVirtualResult, GcodeToolIndex::count, GcodeToolIndex, GcodeToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes>;

    /// Maps GCodeTool -> VirtualTool N:N
    static constexpr GcodeToVirtualArray default_mapping = { stdext::make_iota_array<GcodeToolIndex::count, [](std::size_t i) -> ToVirtualResult {
        return VirtualToolIndex::from_raw(i);
    }>() };

    /// Does not map any GCodeTool to anything
    static constexpr GcodeToVirtualArray empty_mapping = { stdext::make_iota_array<GcodeToolIndex::count, [](std::size_t) -> ToVirtualResult {
        return ToolNotMapped {};
    }>() };

    ToolMapper &operator=(const ToolMapper &other);

    ToolMapper &operator=(const GcodeToVirtualArray &mapping);

    /// Create new mapping of tool
    [[deprecated("Use the ToolIndex overload")]]
    inline bool set_mapping(uint8_t gcode_tool, uint8_t virtual_tool) {
        if (gcode_tool >= GcodeToolIndex::count || virtual_tool >= VirtualToolIndex::count) {
            return false;
        }
        return set_mapping(GcodeToolIndex::from_raw(gcode_tool), VirtualToolIndex::from_raw(virtual_tool));
    }

    /// Create new mapping of tool
    bool set_mapping(GcodeToolIndex gcode_tool, VirtualToolIndex virtual_tool);

    /// Enable or disable all tool mappings
    inline void set_enable(bool enable) {
        std::unique_lock lock(mutex);
        this->enabled = enable;
    }

    /// true when mapping is enabled
    inline bool is_enabled() {
        std::unique_lock lock(mutex);
        return enabled;
    }

    /// Convert gcode tool to virtual
    /// note: might return NO_TOOL_MAPPED, so check for this value
    [[deprecated("Use the ToolIndex overload")]] [[nodiscard]]
    inline uint8_t to_virtual(uint8_t gcode_tool, bool ignore_enabled = false) const {
        if (gcode_tool >= GcodeToolIndex::count) {
            return NO_TOOL_MAPPED;
        }
        auto maybe_virtual = to_virtual(GcodeToolIndex::from_raw(gcode_tool), ignore_enabled);
        return match(
            maybe_virtual,
            [](VirtualToolIndex virtual_tool) { return virtual_tool.to_raw(); },
            [](ToolNotMapped) { return NO_TOOL_MAPPED; });
    }

    /// Convert gcode tool to virtual
    [[nodiscard]] ToVirtualResult to_virtual(GcodeToolIndex gcode_tool, bool ignore_enabled = false) const;

    /// Convert virtual tool to gcode
    [[deprecated("Use the ToolIndex overload")]] [[nodiscard]]
    inline uint8_t to_gcode(uint8_t virtual_tool) const {
        if (virtual_tool >= VirtualToolIndex::count) {
            return NO_TOOL_MAPPED;
        }
        auto maybe_gcode = to_gcode(VirtualToolIndex::from_raw(virtual_tool));
        return match(
            maybe_gcode,
            [](GcodeToolIndex gcode_tool) { return gcode_tool.to_raw(); },
            [](ToolNotMapped) { return NO_TOOL_MAPPED; });
    }

    /// Convert virtual tool to gcode
    [[nodiscard]] inline ToGCodeResult to_gcode(VirtualToolIndex virtual_tool) const {
        std::unique_lock lock(mutex);
        return to_gcode_unlocked(virtual_tool);
    }

    /// Reset all tool mapping
    inline void reset() {
        std::unique_lock lock(mutex);
        gcode_to_virtual = default_mapping;
        enabled = false;
    }

    inline void set_all_unassigned() {
        std::unique_lock lock(mutex);
        for (auto gcode_tool : GcodeToolIndex::all()) {
            set_unassigned_unlocked(gcode_tool);
        }
    }

    inline void set_unassigned(GcodeToolIndex gcode_tool) {
        std::unique_lock lock(mutex);
        set_unassigned_unlocked(gcode_tool);
    }

    /// This is special tool identifier, that says that this tool is not mapped to any tool, and is threfore disabled by tool mapping
    [[deprecated("Use ToolNotMapped from tool_index.hpp")]] static constexpr auto NO_TOOL_MAPPED = std::numeric_limits<uint8_t>::max();

    /// Container with serialized state of tool mapping
    struct __attribute__((packed)) serialized_state_t {
        bool enabled;
        uint8_t gcode_to_virtual[EXTRUDERS];
    };

    /// serialize state into packed structure (for power panic)
    void serialize(serialized_state_t &to);

    /// deserialize state into packed structure (after power panic)
    void deserialize(serialized_state_t &from);

private:
    [[nodiscard]] ToGCodeResult to_gcode_unlocked(VirtualToolIndex virtual_tool) const;

    inline void set_unassigned_unlocked(GcodeToolIndex gcode_tool) {
        gcode_to_virtual[gcode_tool] = ToolNotMapped {};
    }

    mutable freertos::Mutex mutex;
    bool enabled = false;
    GcodeToVirtualArray gcode_to_virtual = default_mapping;
};

extern ToolMapper tool_mapper;
