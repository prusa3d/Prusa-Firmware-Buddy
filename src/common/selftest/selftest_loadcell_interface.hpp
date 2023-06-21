/**
 * @file selftest_loadcell_interface.hpp
 * @author Radek Vana
 * @brief Set of functions adding functionality to selftest classes
 * Could be solved with multiple inheritance (did not want that)
 * or Interface (C++ does not have)
 * @date 2021-09-24
 */
#pragma once
#include "selftest_loadcell_config.hpp"
#include "selftest_loadcell_type.hpp"
#include "Marlin/src/inc/MarlinConfig.h" // HOTENDS
#include <span>

class IPartHandler;
class TestReturn;

namespace selftest {

TestReturn phaseLoadcell(const uint8_t tool_mask, std::array<IPartHandler *, HOTENDS> &m_pLoadcell, const std::span<const LoadcellConfig_t> config);

};
