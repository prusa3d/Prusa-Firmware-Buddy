#pragma once

#include <cstdint>
#include <optional>

namespace indx_nozzle_cleaner_calibration {

void run();

/// Data sent via PhaseData for evaluating phases (must fit in 4 bytes)
struct EvaluatingData {
    /// Measured offset in 0.01 mm units, or @c offset_not_measured when the probe never made
    /// contact (no meaningful value to show).
    int16_t offset_hundredths;
    int16_t nominal_tenths; ///< Nominal position in 0.1 mm units

    /// Sentinel for @c offset_hundredths: no contact was made, so the offset is unknown. Well outside
    /// any physically possible offset, so it can never collide with a real measurement.
    static constexpr int16_t offset_not_measured = INT16_MIN;

    /// @p offset is nullopt when no contact was made (shown as "N/A"), a value otherwise.
    static EvaluatingData from(std::optional<float> offset, float nominal) {
        return {
            .offset_hundredths = offset ? static_cast<int16_t>(*offset * 100.0f) : offset_not_measured,
            .nominal_tenths = static_cast<int16_t>(nominal * 10.0f),
        };
    }

    /// Measured offset [mm], or nullopt when no contact was made.
    std::optional<float> offset() const {
        if (offset_hundredths == offset_not_measured) {
            return std::nullopt;
        }
        return static_cast<float>(offset_hundredths) / 100.0f;
    }
    float nominal() const { return static_cast<float>(nominal_tenths) / 10.0f; }
};
static_assert(sizeof(EvaluatingData) <= 4);

} // namespace indx_nozzle_cleaner_calibration
