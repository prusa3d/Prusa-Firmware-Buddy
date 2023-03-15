/**
 * @file selftest_fans_interface.hpp
 * @author Radek Vana
 * @brief Set of functions adding functionality to selftest classes
 * Could be solved with multiple inheritance (did not want that)
 * or Interface (C++ does not have)
 * @date 2021-09-24
 */
#pragma once
#include <span>
#include "selftest_fan_config.hpp"

class IPartHandler;

namespace selftest {

inline constexpr auto NUM_SELFTEST_FANS = HOTENDS * 2;
bool phaseFans(std::array<IPartHandler *, NUM_SELFTEST_FANS> &fans_parts, const std::span<const FanConfig_t> config_fans);
};
