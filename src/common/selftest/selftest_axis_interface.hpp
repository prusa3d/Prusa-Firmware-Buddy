/**
 * @file selftest_axis_interface.hpp
 * @author Radek Vana
 * @brief Set of functions adding functionality to selftest classes
 * Could be solved with multiple inheritance (did not want that)
 * or Interface (C++ does not have)
 * @date 2021-09-24
 */
#pragma once
#include <cstdint>
#include "selftest_axis_config.hpp"

class IPartHandler;

namespace selftest {
static constexpr size_t axis_count = 3;
extern SelftestSingleAxis_t staticResults[axis_count];

bool phaseAxis(IPartHandler *&m_pAxis, const AxisConfig_t &config_axis);

};
