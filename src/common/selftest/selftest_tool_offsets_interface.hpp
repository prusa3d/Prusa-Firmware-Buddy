#pragma once

#include <array>

#include <i_selftest_part.hpp>
#include "selftest_tool_offsets_config.hpp"
#include "selftest_types.hpp"

namespace selftest {

TestReturn phaseToolOffsets(const ToolMask tool_mask, IPartHandler *&pToolOffsets, const ToolOffsetsConfig_t &config);
TestReturn phaseToolOffset(const ToolMask tool_mask, IPartHandler *&pToolOffsets, const ToolOffsetsConfig_t &config, std::bitset<config_store_ns::max_tool_count> selected_tools);
}; // namespace selftest
