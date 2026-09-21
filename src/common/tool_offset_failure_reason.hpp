/// @file
#pragma once

#include <cstdint>

namespace tool_offset {

/// Why a tool offset calibration attempt produced no usable offset.
///
/// Shared by the measurement routine and the calibration that drives it. The
/// split follows what the user can do about a failure, not where it was
/// detected, so unrelated internal errors share a reason when the remedy is the
/// same. Whatever the reason drops is logged where the failure happens.
///
/// The calibration maps each reason to the prompt the user gets.
enum class FailureReason : uint8_t {
    /// The tool is not picked, so nothing can be cleaned or measured.
    tool_not_picked,

    /// The nozzle cleaning sequence did not finish.
    cleaning_failed,

    /// Probing the bed for the Z reference never triggered.
    bed_probe_failed,

    /// The nozzle stayed too hot to probe at, or reported no temperature.
    nozzle_too_hot,

    homing_failed,

    /// The Z probe never found the sensor surface, or triggered far below it.
    /// Where the sensor is detachable this usually means it is missing.
    sensor_not_found,

    /// The sensor stopped delivering usable samples. Cleaning does not help.
    sensor_failure,

    /// The sweeps ran and the sensor answered, but the nozzle was never located
    /// confidently. Usually a dirty nozzle or a dirty sensor.
    not_measured,
};

} // namespace tool_offset
