#include "homing_modus.hpp"
#include <cmath>

/// @brief compute nearest (signed) distance to calibrated value
/// in 1024 circle
///
/// @return -512 .. +512
int to_calibrated(const int calibrated, const int value) {
    const int diff = calibrated - value;
    if (std::abs(diff) <= 512) {
        return diff;
    }
    if (diff < 0) {
        return 1024 + diff;
    }
    return diff - 1024;
}

int home_modus(uint16_t *positions, uint32_t samples, uint32_t range) {
    /// weighted modus
    int max = 0;
    /// modus
    int max_n = 0;
    int max_pos = 0;
    int sum;
    int sum_n;
    int weight;
    /// number of places with same values
    int same = 1;

    for (uint32_t i = 0; i < 1024; ++i) {
        sum = 0;
        sum_n = 0;
        for (uint32_t j = 0; j < samples; ++j) {
            weight = std::max(0, 1 + (int)range - std::abs(to_calibrated(i, positions[j])));
            sum += weight;
            /// count number of values in the range
            sum_n += std::min(1, weight);
        }

        if (sum > max || (sum == max && sum_n > max_n)) {
            max = sum;
            max_n = sum_n;
            max_pos = i;
            same = 1;
        } else if (sum == max && sum_n == max_n) {
            ++same;
        }
    }
    return (max_pos + (same - 1) / 2) % 1024;
}
