/**
 * @file selftest_fans_interface.hpp
 * @author Radek Vana
 * @brief Set of functions adding functionality to selftest classes
 * Could be solved with multiple inheritance (did not want that)
 * or Interface (C++ does not have)
 * @date 2021-09-24
 */
#pragma once
#include "selftest_fan_config.hpp"

class IPartHandler;

namespace selftest {

bool phaseFans(IPartHandler *&pPrintFan, IPartHandler *&pHeatbreakFan, const FanConfig_t &config_print_fan, const FanConfig_t &config_heatbreak_fan);

};
