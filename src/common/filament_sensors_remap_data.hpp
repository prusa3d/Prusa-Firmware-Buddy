#pragma once

#include <array>
#include <config_store/constants.hpp>

namespace side_fsensor_remap {

/**
 * @note Mapping of tools to sensor positions.
 *   mapping[extruder] = sensor_position
 * @warning This type is used in config store and must not be changed.
 */
using Mapping = std::array<uint8_t, config_store_ns::max_tool_count>;

namespace preset {
    template <size_t... Is>
    constexpr std::array<uint8_t, sizeof...(Is)> generate_normal_mapping(std::index_sequence<Is...>) {
        //  this is just fancy template way to init array in initializer_list
        return { (Is)... };
    }

    /**
     * @brief Default layout when not remapped is a plain integer sequence.
     * @warning This value is used in config store and should not be changed.
     */
    inline constexpr Mapping no_mapping { generate_normal_mapping(std::make_index_sequence<std::tuple_size<Mapping> {}>()) };

} // namespace preset

} // namespace side_fsensor_remap
