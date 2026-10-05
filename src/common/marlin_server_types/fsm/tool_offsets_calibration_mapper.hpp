/// \file

#pragma once

#include <optional>

#include <client_response.hpp>
#include <error_codes.hpp>
#include <option/has_tool_offset_sensor.h>

#if HAS_TOOL_OFFSET_SENSOR()
constexpr std::optional<ErrCode> tool_offsets_calibration_phase_error_code_mapper(const FSMAndPhase fsm_phase) {
    switch (static_cast<PhaseToolOffsetsCalibration>(fsm_phase.phase)) {
    case PhaseToolOffsetsCalibration::pickup_failed:
        return ErrCode::ERR_MECHANICAL_NOZZLE_MISMATCH_PICKUP_FAILED;
    case PhaseToolOffsetsCalibration::tool_not_picked:
        return ErrCode::ERR_MECHANICAL_TOOL_NOT_PICKED_RETRY;
    case PhaseToolOffsetsCalibration::sensor_not_found:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_SENSOR_NOT_FOUND;
    case PhaseToolOffsetsCalibration::sensor_failure:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_SENSOR_MEASUREMENT_FAILED;
    case PhaseToolOffsetsCalibration::tool_failed:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_CALIBRATION_FAILED;
    case PhaseToolOffsetsCalibration::unsafe_xy_offset:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_UNSAFE_XY;
    case PhaseToolOffsetsCalibration::unsafe_z_offset:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_UNSAFE_Z_DEVIATION;
    case PhaseToolOffsetsCalibration::unsafe_sensor_position:
        return ErrCode::ERR_MECHANICAL_TOOL_OFFSET_UNSAFE_SENSOR_XY;
    default:
        return std::nullopt;
    }
}
#endif
