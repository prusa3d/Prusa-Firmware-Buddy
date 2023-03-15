#pragma once

#include <array>

#include <i_selftest_part.hpp>

namespace selftest {

bool phaseKennels(const uint8_t tool_mask, std::array<IPartHandler *, HOTENDS> &pKennels, const std::array<const KennelConfig_t, HOTENDS> &configs);

};
