/**
 * @file selftest_firstlayer_config.hpp
 */

#pragma once
#include <cstdint>
#include "selftest_firstlayer_type.hpp"
#include "client_response.hpp"

namespace selftest {
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct FirstLayerConfig_t {
    using type_evaluation = SelftestFirstLayer_t;
    static constexpr SelftestParts part_type = SelftestParts::FirstLayerQuestions;
    const char *partname;
};

}; // namespace selftest
