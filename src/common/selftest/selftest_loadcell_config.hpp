/**
 * @file selftest_loadcell_config.hpp
 * @author Radek Vana
 * @brief config loadcell selftest part
 * @date 2021-10-13
 */

#pragma once
#include <cstdint>
#include "fanctl.hpp"
#include "client_response.hpp"
#include "selftest_loadcell_type.hpp"

namespace selftest {
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct LoadcellConfig_t {
    using type_evaluation = SelftestLoadcell_t;
    using FanCtlFnc = CFanCtl &(*)(size_t);
    static constexpr SelftestParts part_type = SelftestParts::Loadcell;
    const char *partname;
    uint8_t tool_nr;
    FanCtlFnc heatbreak_fan_fnc;
    FanCtlFnc print_fan_fnc;
    int32_t cool_temp;
    uint32_t countdown_sec;
    int32_t countdown_load_error_value;
    int32_t tap_min_load_ok;
    int32_t tap_max_load_ok;
    uint32_t tap_timeout_ms;
    uint32_t z_extra_pos;
    uint32_t z_extra_pos_fr;
    uint32_t max_validation_time;
};

}; // namespace selftest
