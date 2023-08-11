#pragma once

#include "selftest_gears_result.hpp"

namespace selftest {

struct SelftestGearsConfig {
    using type_evaluation = SelftestGearsResult;
    static constexpr SelftestParts part_type = SelftestParts::GearsCalib;
    const char *partname = "gear";

    float feedrate = 0;
};

}; // namespace selftest
