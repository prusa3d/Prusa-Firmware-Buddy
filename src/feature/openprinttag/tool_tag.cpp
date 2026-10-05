/// @file
#include "tool_tag.hpp"

namespace buddy::openprinttag {

static_assert(sizeof(ToolTag) == 4);

TagUID::Hash TagUID::hash() const {
    uint16_t h = (data[0] << 8 | data[1])
        ^ (data[2] << 8 | data[3])
        ^ (data[4] << 8 | data[5])
        ^ (data[6] << 8 | data[7]);
    return h | 1; // make sure to never return no_tag_hash
}

} // namespace buddy::openprinttag
