/**
 * @file selftest_esp_config.hpp
 * @brief config esp selftest part
 */

#pragma once
#include <cstdint>
#include "selftest_esp_type.hpp"
#include "client_response.hpp"

namespace selftest {
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct EspConfig_t {
    using type_evaluation = SelftestESP_t;
    static constexpr SelftestParts part_type = SelftestParts::ESP;
    const char *partname;
};

}; // namespace selftest
