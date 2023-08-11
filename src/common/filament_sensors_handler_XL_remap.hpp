#pragma once

#include "filament_sensors_remap_data.hpp"

namespace side_fsensor_remap {

namespace preset {
    /// @note Default no_mapping is defined in "filament_sensors_remap_data.hpp".

    static_assert(config_store_ns::max_tool_count == 6, "XL presets work for 6 tools only");

    /**
     * @brief Remap tool 2 right and move tool 3 and 4 one down.
     * For more than 3 tools where right sensor block is necessary.
     * Left    Right
     *  T0      T2
     *  T1      T3
     *  _       T4
     */
    inline constexpr Mapping has_right = {
        0, // T0    - left top
        1, // T1    - left middle
        3, // T2    - right top
        4, // T3    - right middle
        5, // T4    - right bottom
        2, // Empty - left bottom
    };

    /**
     * @brief Remap all 3.
     * For less than or 3 tools all left.
     * Left    Right
     *  _       T0
     *  _       T1
     *  _       T2
     */
    inline constexpr Mapping left_only = {
        3, // T0    - right top
        4, // T1    - right middle
        5, // T2    - right bottom
        0, // Empty - left top
        1, // Empty - left middle
        2, // Empty - left bottom
    };
} // namespace preset

/**
 * @brief Know if this printer has more than 3 tools.
 * @return true if more than 3 tools
 */
bool has_right_side_sensors();

/**
 * @brief Know remap state.
 * @return true if any remap is enabled
 */
bool is_remapped();

/**
 * @brief Enable or disable remap.
 * @param mapping new mapping
 * Use set_map(no_mapping) to clear to default.
 */
void set_mapping(const Mapping &mapping);

/**
 * @brief Get current mapping.
 * @return array with mapping
 */
const Mapping get_mapping();

/**
 * @brief Get simple remap, just on/off.
 * @param enable true to enable remap
 * @return array with mapping
 */
const Mapping &get_simple_mapping(bool enable);

/**
 * @brief Ask user to remap.
 * This uses MsgBoxQuestion and should be used only from GUI thread.
 * @return mask of remapped tools, 0 if mapping didn't change
 */
uint32_t ask_to_remap();

} // namespace side_fsensor_remap
