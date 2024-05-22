#pragma once

#include <array>

#include <i_selftest_part.hpp>
#include <selftest_types.hpp>
#include <selftest_dock_config.hpp>

namespace selftest {

TestReturn phaseDocks(const ToolMask tool_mask, std::array<IPartHandler *, HOTENDS> &pDocks, const std::array<const DockConfig_t, HOTENDS> &configs);

};
