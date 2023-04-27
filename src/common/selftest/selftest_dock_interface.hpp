#pragma once

#include <array>

#include <i_selftest_part.hpp>

namespace selftest {

bool phaseDocks(const uint8_t tool_mask, std::array<IPartHandler *, HOTENDS> &pDocks, const std::array<const DockConfig_t, HOTENDS> &configs);

};
