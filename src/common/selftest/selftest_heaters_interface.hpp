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
#include "selftest_hot_end_sock_config.hpp"
#include "selftest_hot_end_sock_type.hpp"
#include <span>

class IPartHandler;

namespace selftest {

void phaseHeaters_noz_ena(std::array<IPartHandler *, HOTENDS> &pNozzles, const std::span<const HeaterConfig_t> config_nozzle);
void phaseHeaters_bed_ena(IPartHandler *&pBed, const HeaterConfig_t &config_bed);
bool phaseHeaters(std::array<IPartHandler *, HOTENDS> &pNozzles, IPartHandler **pBed);
bool phase_hot_end_sock(IPartHandler *&machine, const HotEndSockConfig &config);
bool get_retry_heater();
}; // namespace selftest
