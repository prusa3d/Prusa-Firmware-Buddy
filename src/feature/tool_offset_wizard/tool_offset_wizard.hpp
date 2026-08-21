#pragma once

#include <cstdint>

namespace tool_offset_wizard {

void run();

/// Data sent via PhaseData for the `calibrating` phase (must fit in 4 bytes)
struct ProgressData {
    uint8_t step; ///< 1-based step counter (1..total_steps)
    uint8_t total_steps; ///< number of tools being calibrated
    uint8_t tool_raw; ///< PhysicalToolIndex::to_raw() of the tool currently being calibrated

    static ProgressData from(uint8_t step, uint8_t total, uint8_t tool) {
        return { .step = step, .total_steps = total, .tool_raw = tool };
    }
};
static_assert(sizeof(ProgressData) <= 4);

} // namespace tool_offset_wizard
