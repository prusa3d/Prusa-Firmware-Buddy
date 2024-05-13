#include "fnt-indices.hpp"
#include <algorithm>

// use pre-generated font_indices.ipp
struct FCIndex {
    uint16_t unc; /// utf8 character value (stripped of prefixes)
    uint8_t charX, charY;
};
// clang-format off
static constexpr const FCIndex fontCharIndices[] =
#include "../guiapi/include/fnt-full-indices.ipp"
    static constexpr const uint32_t fontCharIndicesNumItems = sizeof(fontCharIndices) / sizeof(FCIndex);
// clang-format on

bool NonASCIICharKnown(unichar c) {
    auto i = std::lower_bound(fontCharIndices, fontCharIndices + fontCharIndicesNumItems, c,
        [](auto fcIndex, auto c) -> bool {
            return fcIndex.unc < c;
        });

    return i != fontCharIndices + fontCharIndicesNumItems ? i->unc == c : false;
}
