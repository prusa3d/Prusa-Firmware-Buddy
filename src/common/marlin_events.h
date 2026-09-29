// marlin_events.h
#pragma once

#include <cstdint>
#include <utils/utility_extensions.hpp>

namespace marlin_server {

enum class Event : uint8_t {
    RequestCalibrationsScreen,
    Acknowledge,
    NotAcknowledge,

    _count, // keep last
};

using EventMask = uint32_t;

// Event group reserves the top 8 bits
static_assert(std::to_underlying(Event::_count) <= 24);

// event masks
constexpr EventMask make_mask(Event id) {
    return static_cast<EventMask>(1) << std::to_underlying(id);
}

// commands
enum class Cmd : uint32_t {
    NONE = 0,
    G = (uint32_t)'G' << 16,
    M = (uint32_t)'M' << 16,
    G28 = G + 28,
    G29 = G + 29,
    M109 = M + 109,
    M190 = M + 190,
    M303 = M + 303,
    M600 = M + 600,
    M701 = M + 701,
    M702 = M + 702,
    M876 = M + 876,
};

/**
 * @brief Parameter to start printing.
 * Tells whether to skip parts of preview when printing is started.
 * @warning The order matters. Element skips its screen and all screens with lower number.
 */
enum class PreviewSkipIfAble : uint8_t {
    no = 0, ///< Show all
    preview, ///< Skip preview thumbnail
    tool_mapping, ///< Skip preview thumbnail and toolmapping
    _count,
    _last = _count - 1,
    all = _last, ///< Skip all
};

/// Used in print_start - denotes whether tool mapping should be kept from the previous print or not
enum class ResetToolMapping : bool {
    no = false,
    yes = true,
};

} // namespace marlin_server
