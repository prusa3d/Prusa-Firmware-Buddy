/// \file
#pragma once

#include <cstdint>
#include <algorithm>
#include <bsod/bsod.h>

using ProgressPercent = uint8_t;

struct ProgressSpan {
    ProgressPercent min = 0;
    ProgressPercent max = 100;

    constexpr ProgressPercent map(float normalized_progress) const {
        debug_assert(normalized_progress >= 0 && normalized_progress <= 1);
        const float percent = min + normalized_progress * (max - min);
        return static_cast<ProgressPercent>(percent);
    }

    constexpr bool operator==(const ProgressSpan &o) const = default;
};

/// Maps \param value from range \param min - \param max to range [0, 1]
inline float to_normalized_progress(const float min, const float max, const float value) {
    // Handle edge case where min >= max (avoid division by zero)
    const float range = max - min;
    if (range <= 1e-6f) {
        return 1.0f;
    }

    return std::clamp((value - min) / range, 0.0f, 1.0f);
}
