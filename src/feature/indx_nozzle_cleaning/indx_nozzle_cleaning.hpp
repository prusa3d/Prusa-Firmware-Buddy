#pragma once

#include <cstdint>

#include <common/fsm_base_types.hpp>

namespace indx_nozzle_cleaning {

enum class Mode : uint8_t {
    /// Heat up, run the cleaning sequence on the nozzle cleaner
    automatic,

    /// Heat up, let the user clean the nozzle hot, cool down, let the user finish, run the cleaning sequence
    manual,
};

/// Runs the nozzle cleaning wizard on the tools in @p tool_mask (bit N = physical tool N).
/// Disabled tools are skipped.
void run(uint32_t tool_mask, Mode mode);

/// Data sent via fsm::PhaseData. Bytes 0-1 hold the nozzle temperature big-endian (as FrameWaitTemp expects).
struct PhaseDataLayout {
    uint8_t temperature_hi;
    uint8_t temperature_lo;
    uint8_t tool_display_index;

    static fsm::PhaseData make(uint8_t tool_display_index, int16_t temperature) {
        const uint16_t t = static_cast<uint16_t>(temperature < 0 ? 0 : temperature);
        return fsm::serialize_data(PhaseDataLayout {
            .temperature_hi = static_cast<uint8_t>(t >> 8),
            .temperature_lo = static_cast<uint8_t>(t & 0xff),
            .tool_display_index = tool_display_index,
        });
    }
};
static_assert(sizeof(PhaseDataLayout) <= sizeof(fsm::PhaseData));

} // namespace indx_nozzle_cleaning
