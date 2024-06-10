/**
 * @file selftest_fan_config.hpp
 * @brief config fan selftest part
 */

#pragma once
#include <cstdint>
#include "client_response.hpp"
#include "fanctl.hpp"
#include "selftest_fans_type.hpp"

namespace selftest {

struct FanConfig {
    uint16_t rpm_min;
    uint16_t rpm_max;
    uint16_t ignore_min_overlap;
};

constexpr selftest::FanConfig benevolent_fan_config = { .rpm_min = 10, .rpm_max = 10000, .ignore_min_overlap = 0 };

struct SelftestFansConfig {
    using type_evaluation = SelftestFanHotendResult;

    static constexpr SelftestParts part_type = SelftestParts::Fans;

    uint8_t tool_nr { 0 };

    FanConfig print_fan;
    FanConfig heatbreak_fan;
};

} // namespace selftest
