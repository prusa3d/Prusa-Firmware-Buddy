/**
 * @file selftest_axis_interface.hpp
 * @author Radek Vana
 * @brief Set of functions adding functionality to selftest classes
 * Could be solved with multiple inheritance (did not want that)
 * or Interface (C++ does not have)
 * @date 2021-09-24
 */
#pragma once
#include "selftest_fsensor_config.hpp"
#include "selftest_types.hpp"

class IPartHandler;
class TestReturn;

namespace selftest {

/**
 * @brief run filament sensor test
 *
 * @param m_pFSensor reference to pointer to test instance
 * @param config configuration file
 * @return true stay in current selftest state
 * @return false continue to next selftest state
 */
TestReturn phaseFSensor(const ToolMask tool_mask, std::array<IPartHandler *, HOTENDS> &m_pFSensor, const std::array<const FSensorConfig_t, HOTENDS> &configs);
}; // namespace selftest
