#pragma once

#include <cstdint>
#include "selftest_dock_type.hpp"

namespace selftest {
struct DockConfig_t {
    using type_evaluation = SelftestDock_t;
    static constexpr SelftestParts part_type = SelftestParts::Dock;
    uint8_t dock_id = 0;
};

};
