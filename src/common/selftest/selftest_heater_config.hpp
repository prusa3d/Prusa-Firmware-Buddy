/**
 * @file selftest_heater_config.hpp
 * @author Radek Vana
 * @brief config heater selftest parts
 * @date 2021-10-11
 */

#pragma once
#include <cstdint>
#include "fanctl.h"
#include "client_response.hpp"
#include "selftest_heaters_type.hpp"

namespace selftest {
//using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct HeaterConfig_t {
    using type_evaluation = SelftestHeater_t;
    static constexpr SelftestParts part_type = SelftestParts::Heaters;
    using temp_getter = float (*)();
    using temp_setter = void (*)(int);
    const char *partname;
    temp_getter getTemp;
    temp_setter setTargetTemp;
    float &refKp;
    float &refKi;
    float &refKd;
    CFanCtl &heatbreak_fan;
    CFanCtl &print_fan;
    uint32_t heat_time_ms;
    int32_t start_temp;
    int32_t undercool_temp;
    int32_t target_temp;
    int32_t heat_min_temp;
    int32_t heat_max_temp;

    uint32_t heater_load_stable_ms;
    float heater_full_load_min_W;
    float heater_full_load_max_W;
    uint32_t pwm_100percent_equivalent_value;
    uint32_t min_pwm_to_measure;
};

};
