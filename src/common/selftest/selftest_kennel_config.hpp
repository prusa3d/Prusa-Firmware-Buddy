#pragma once

#include <cstdint>
#include "selftest_kennel_type.hpp"

namespace selftest {
struct KennelConfig_t {
    using type_evaluation = SelftestKennel_t;
    static constexpr SelftestParts part_type = SelftestParts::Kennel;
    uint8_t kennel_id = 0;
};

};
