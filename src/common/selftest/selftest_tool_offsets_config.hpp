#pragma once

#include <cstdint>
#include "selftest_tool_offsets_type.hpp"

namespace selftest {
struct ToolOffsetsConfig_t {
    using type_evaluation = SelftestToolOffsets_t;
    static constexpr SelftestParts part_type = SelftestParts::ToolOffsets;
};

}; // namespace selftest
