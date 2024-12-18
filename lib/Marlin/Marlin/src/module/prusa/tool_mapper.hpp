#pragma once

#include <freertos/mutex.hpp>
#include "inc/MarlinConfig.h"
#include <limits>
#include <mutex>

#if ENABLED(PRUSA_TOOL_MAPPING)

/**
 * @brief Tool mapper allows user to assign tools in GUI/GCODE to other tools, depending on where he has filament he wants to use
 *
 * @note  Gcode tool - Tool that gcode refers to (eg. T1)
 *        Physical tool - Actual physical tool on printer
 *        Mapping of gcode tool 1 to physical tool 3 means that whenever gcode requests tool 1, printer will actually use tool 3
 */
class ToolMapper {
public:
    ToolMapper();

    ToolMapper &operator=(const ToolMapper &other);

    /// Create new mapping of tool
    bool set_mapping(uint8_t gcode, uint8_t physical);

    /// Enable or disable all tool mappings
    void set_enable(bool enable);

    /// true when mapping is enabled
    inline bool is_enabled() {
        std::unique_lock lock(mutex);
        return enabled;
    }

    /// Convert gcode tool to physical
    /// note: might return NO_TOOL_MAPPED, so check for this value
    [[nodiscard]] uint8_t to_physical(uint8_t gcode, bool ignore_enabled = false) const;

    /// Convert physical tool to gcode
    [[nodiscard]] uint8_t to_gcode(uint8_t physical) const;

    /// Reset all tool mapping
    void reset();

    void set_all_unassigned();
    bool set_unassigned(uint8_t gcode);

    // This is special tool identifier, that says that this tool is not mapped to any tool, and is threfore disabled by tool mapping
    static constexpr auto NO_TOOL_MAPPED = std::numeric_limits<uint8_t>::max();

    // Container with serialized state of tool mapping
    struct __attribute__((packed)) serialized_state_t {
        bool enabled;
        uint8_t gcode_to_physical[EXTRUDERS];
    };

    // serialize state into packed structure (for power panic)
    void serialize(serialized_state_t &to);

    // deserialize state into packed structure (after power panic)
    void deserialize(serialized_state_t &from);

private:
    [[nodiscard]] uint8_t to_gcode_unlocked(uint8_t physical) const;
    bool set_unassigned_unlocked(uint8_t logical);

    mutable freertos::Mutex mutex;
    bool enabled;
    uint8_t gcode_to_physical[EXTRUDERS];
};

extern ToolMapper tool_mapper;

#endif
