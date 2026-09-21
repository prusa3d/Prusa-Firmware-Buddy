#pragma once
#include <contactless_offset/tool_sensor.hpp>
#include "clo_config.hpp"
#include <expected>
#include <tool_offset_failure_reason.hpp>

namespace tool_offset {

struct ToolOffset {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Measure the current tool's XYZ offset relative to the sensor reference position.
// Performs homing, Z probing, and XY scanning internally. The geometry in
// `config` selects single-coil (hunt X+Y over one coil) or dual-coil (XLS:
// X over one coil, Y over the other, each probed separately).
// Failures are logged here; the caller gets only the reason.
std::expected<ToolOffset, FailureReason> measure_current_tool_offset(
    const ProbingConfig &config,
    const ToolOffset &actual_offset);

} // namespace tool_offset
