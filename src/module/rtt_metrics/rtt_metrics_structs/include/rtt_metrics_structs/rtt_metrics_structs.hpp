///@file
#pragma once

#include <cstddef>
#include <cstdint>

namespace rtt_metrics {
enum class MetricType : uint8_t {
    raw_acceleration,
    loadcell_tared_z,
    stepper_positions,
};

template <typename DataStruct, MetricType TYPE>
struct MetricWrapper {
    static constexpr MetricType type = TYPE;
    uint32_t timestamp;
    DataStruct data;
};

/// Payload for MetricType::loadcell_tared_z: the tared Z load
/// (Loadcell::get_tared_z_load) in grams. Already scaled and calibrated on the
/// firmware; tare-relative, so a load change rather than an absolute weight.
struct LoadcellTaredZ {
    float z_load;
};

/// Number of stepper positions carried by StepperPositions. This is part of
/// the wire format shared with host-side decoders, so it is a fixed constant
/// rather than a Marlin config value (keeping these structs Marlin-free and
/// host-compilable). Firmware static_asserts it against XYZE_N.
inline constexpr size_t stepper_count = 4;

/// Payload for MetricType::stepper_positions: per-stepper position in steps
/// since startup (Stepper::position_from_startup), indexed by AxisEnum.
struct StepperPositions {
    int32_t steps[stepper_count];
};

} // namespace rtt_metrics
