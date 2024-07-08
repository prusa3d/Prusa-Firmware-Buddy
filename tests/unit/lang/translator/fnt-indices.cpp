#include "fnt-indices.hpp"
#include <algorithm>

static constexpr const uint16_t fontCharIndices[] = {
#include "../guiapi/include/fnt-full-indices.ipp"
};
static constexpr const uint32_t fontCharIndicesNumItems = sizeof(fontCharIndices) / sizeof(uint16_t);

bool NonASCIICharKnown(unichar c) {
    auto i = std::lower_bound(fontCharIndices, fontCharIndices + fontCharIndicesNumItems, c);
    return i != fontCharIndices + fontCharIndicesNumItems ? *i == c : false;
}
