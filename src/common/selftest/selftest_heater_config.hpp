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
#include <option/has_heaters_selftest_gcode.h>
#include "selftest_heaters_type.hpp"
#include <tool/hotend/hotend.hpp>

#if PRINTER_IS_PRUSA_XL()
    #define HAS_SELFTEST_POWER_CHECK()        1
    #define HAS_SELFTEST_POWER_CHECK_SINGLE() 1
#else
    #define HAS_SELFTEST_POWER_CHECK()        0
    #define HAS_SELFTEST_POWER_CHECK_SINGLE() 0
#endif

namespace selftest {

enum class heater_type_t {
    Nozzle,
    Bed,
};
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct HeaterConfig_t {
    using type_evaluation = SelftestHeater_t;
    using FanCtlFnc = CFanCtlCommon &(*)(PhysicalToolIndex);
    static constexpr SelftestParts part_type = SelftestParts::Heaters;
    using temp_getter = Hotend::OptionalTemperature (*)();
    using temp_setter = void (*)(int);
    const char *partname;
    heater_type_t type;
    PhysicalToolIndex tool_nr = PhysicalToolIndex::from_raw(0);
    temp_getter getTemp;
    temp_setter setTargetTemp;
    PID_t (*get_pid)();
    void (*set_pid)(const PID_t &);
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
    uint32_t min_pwm_to_measure { 0 };

    /// INDX nozzle only: deadline for reaching target-temp residency. The thermal protections
    /// (thermal model, heater watch) trip long before this on a broken heater; the deadline is
    /// just a backstop, so it can be generous.
    uint32_t heat_timeout_ms { 0 };
};

#if HAS_HEATERS_SELFTEST_GCODE()
// Per-variant heater configs for the gcode-based heater selftest (M1987). Each non-XL
// selftest_<VARIANT>.cpp defines these, reusing its existing Config_Heater* values.
HeaterConfig_t nozzle_heater_config();
HeaterConfig_t bed_heater_config();
#endif

}; // namespace selftest
