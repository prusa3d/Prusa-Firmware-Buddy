#pragma once

#include <array>

#include <i_selftest_part.hpp>
#include "selftest_tool_offsets_config.hpp"

namespace selftest {

TestReturn phaseToolOffsets(const uint8_t tool_mask, IPartHandler *&pToolOffsets, const ToolOffsetsConfig_t &config);

};
