/**
 * @file eeprom_current.hpp
 * @brief current version of eeprom
 */
#pragma once

#include "eeprom_v22.hpp"
#include "selftest_result.hpp"
namespace config_store_ns::old_eeprom::current {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of current eeprom
 * without head and crc
 */
struct vars_body_t : public old_eeprom::v22::vars_body_t {
    SelftestResultV23 SELFTEST_RESULT;
    bool NOZZLE_SOCK;
    uint8_t NOZZLE_TYPE;
};

#pragma pack(pop)

constexpr vars_body_t body_defaults = {
    old_eeprom::v22::body_defaults,
    {}, // EEVAR_SELFTEST_RESULT_V_23
    false, // EEVAR_NOZZLE_SOCK
    0, // EEVAR_NOZZLE_TYPE
};

inline vars_body_t convert(const old_eeprom::v22::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v22 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v22::vars_body_t));

    // SelftestResult can't have a constructor (it's being included in C)
    // since upgrade shall be made only at this place, I have just coded it here
    ret.SELFTEST_RESULT.xaxis = ret.SELFTEST_RESULT_PRE_23.xaxis;
    ret.SELFTEST_RESULT.yaxis = ret.SELFTEST_RESULT_PRE_23.yaxis;
    ret.SELFTEST_RESULT.zaxis = ret.SELFTEST_RESULT_PRE_23.zaxis;
    ret.SELFTEST_RESULT.bed = ret.SELFTEST_RESULT_PRE_23.bed;
    ret.SELFTEST_RESULT.eth = ret.SELFTEST_RESULT_PRE_23.eth;
    ret.SELFTEST_RESULT.wifi = ret.SELFTEST_RESULT_PRE_23.wifi;
    ret.SELFTEST_RESULT.zalign = ret.SELFTEST_RESULT_PRE_23.zalign;
    for (size_t e = 0; e < old_eeprom::EEPROM_MAX_TOOL_COUNT; e++) {
        ret.SELFTEST_RESULT.tools[e].printFan = ret.SELFTEST_RESULT_PRE_23.tools[e].printFan;
        ret.SELFTEST_RESULT.tools[e].heatBreakFan = ret.SELFTEST_RESULT_PRE_23.tools[e].heatBreakFan;
        ret.SELFTEST_RESULT.tools[e].nozzle = ret.SELFTEST_RESULT_PRE_23.tools[e].nozzle;
        ret.SELFTEST_RESULT.tools[e].fsensor = ret.SELFTEST_RESULT_PRE_23.tools[e].fsensor;
        ret.SELFTEST_RESULT.tools[e].loadcell = ret.SELFTEST_RESULT_PRE_23.tools[e].loadcell;
        ret.SELFTEST_RESULT.tools[e].sideFsensor = ret.SELFTEST_RESULT_PRE_23.tools[e].sideFsensor;
        ret.SELFTEST_RESULT.tools[e].dockoffset = ret.SELFTEST_RESULT_PRE_23.tools[e].dockoffset;
        ret.SELFTEST_RESULT.tools[e].tooloffset = ret.SELFTEST_RESULT_PRE_23.tools[e].tooloffset;
    }

    return ret;
}

} // namespace config_store_ns::old_eeprom::current
