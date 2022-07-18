/**
 * @file selftest_fan_config.hpp
 * @brief config fan selftest part
 */

#pragma once
#include <cstdint>
#include "client_response.hpp"
#include "fanctl.h"
#include "selftest_fans_type.hpp"

namespace selftest {
//using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct FanConfig_t {
    using type_evaluation = SelftestFan_t;
    static constexpr SelftestParts part_type = SelftestParts::Fans;
    const char *partname;
    CFanCtl &fanctl;
    int pwm_start;
    int pwm_step;
    const uint16_t *rpm_min_table;
    const uint16_t *rpm_max_table;
    size_t steps;
};

};
