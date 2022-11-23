/**
 * @file selftest_heaters_interface.hpp
 * @author Radek Vana
 * @brief Set of functions adding functionality to selftest classes
 * Could be solved with multiple inheritance (did not want that)
 * or Interface (C++ does not have)
 * @date 2021-09-24
 */
#pragma once
#include "selftest_heater_config.hpp"

class CFanCtl;
class IPartHandler;

namespace selftest {

void phaseHeaters_noz_ena(IPartHandler *&pNozzle, const HeaterConfig_t &config_nozzle);
void phaseHeaters_bed_ena(IPartHandler *&pBed, const HeaterConfig_t &config_bed);
bool phaseHeaters(IPartHandler *&pNozzle, IPartHandler *&pBed);

};
