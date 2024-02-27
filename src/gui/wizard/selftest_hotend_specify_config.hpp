/**
 * @file selftest_hotend_specify_config.hpp
 */

#pragma once
#include <cstdint>
#include "selftest_hotend_specify_type.hpp"
#include "client_response.hpp"

namespace selftest {
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct HotendSpecifyConfig {
    using type_evaluation = SelftestHotendSpecifyType;
    static constexpr SelftestParts part_type = SelftestParts::SpecifyHotend;
    const char *partname;
};

}; // namespace selftest
