/**
 * @file selftest_axis_config.hpp
 * @author Radek Vana
 * @brief config axis selftest parts
 * @date 2021-10-11
 */

#pragma once
#include <cstdint>
#include "client_response.hpp"
#include "selftest_axis_type.hpp"

namespace selftest {
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
// TODO: If axis would be AxisEnum the code would be much less error prone at cost of extra shift operation in value access
struct AxisConfig_t {
    using type_evaluation = SelftestSingleAxis_t;
    static constexpr SelftestParts part_type = SelftestParts::Axis;
    const char *partname;
    float length;
    const float *fr_table_fw; // forward
    const float *fr_table_bw; // backward
    float length_min;
    float length_max;
    AxisEnum axis; // AxisEnum
    uint32_t steps;
    int32_t movement_dir; // not motor dir, has values 1/-1. TODO FIXME -1 does not work
    bool park; ///< If true, park the axis after the test
    float park_pos; ///< Position to park the axis [mm]
};

}; // namespace selftest
