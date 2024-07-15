/**
 * @file selftest_heater_config.hpp
 * @author Radek Vana
 * @brief config heater selftest parts
 * @date 2021-10-11
 */

#pragma once
#include <cstdint>
#include "fanctl.hpp"
#include "client_response.hpp"
#include "selftest_heaters_type.hpp"

namespace selftest {

enum class heater_type_t {
    Nozzle,
    Bed,
};
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct HeaterConfig_t {
    using type_evaluation = SelftestHeater_t;
    using FanCtlFnc = CFanCtlCommon &(*)(size_t);
    static constexpr SelftestParts part_type = SelftestParts::Heaters;
    using temp_getter = float (*)();
    using temp_setter = void (*)(int);
    const char *partname;
    heater_type_t type;
    uint8_t tool_nr { 0 }; // when type=Nozzle, this will contain related nozzle number
    temp_getter getTemp;
    temp_setter setTargetTemp;
    float &refKp;
    float &refKi;
    float &refKd;
    FanCtlFnc heatbreak_fan_fnc;
    FanCtlFnc print_fan_fnc;
    uint32_t heat_time_ms;
    int32_t start_temp;
    int32_t undercool_temp;
    int32_t target_temp;
    int32_t heat_min_temp;
    int32_t heat_max_temp;
    int32_t heatbreak_min_temp { 0 };
    int32_t heatbreak_max_temp { 0 };

    uint32_t heater_load_stable_ms { 0 };
    static constexpr int32_t heater_load_stable_difference { 3 };
    float heater_full_load_min_W { 0 };
    float heater_full_load_max_W { 0 };
    uint32_t pwm_100percent_equivalent_value { 0 };
    uint32_t min_pwm_to_measure { 0 };

    std::array<int8_t, static_cast<size_t>(HotendType::_cnt)> hotend_type_temp_offsets { 0 };
};

}; // namespace selftest
