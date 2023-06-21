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

template <size_t STEPS>
struct FanConfig {
    using FanCtlFnc = CFanCtl &(*)(size_t);

    uint8_t pwm_start { 0 };
    uint8_t pwm_step { 0 };
    std::array<uint16_t, STEPS> rpm_min_table;
    std::array<uint16_t, STEPS> rpm_max_table;

    FanCtlFnc fanctl_fnc;
};

struct SelftestFansConfig {
    using type_evaluation = SelftestFanHotendResult;

    static constexpr SelftestParts part_type = SelftestParts::Fans;

    static constexpr size_t steps { 2 };

    uint8_t tool_nr { 0 };

    FanConfig<steps> print_fan;
    FanConfig<steps> heatbreak_fan;
};

} // namespace selftest
