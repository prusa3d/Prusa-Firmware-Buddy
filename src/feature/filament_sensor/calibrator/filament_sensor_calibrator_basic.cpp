#include "filament_sensor_calibrator_basic.hpp"

#include <logging/log.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(FSensor);

FilamentSensorCalibratorBasic::FilamentSensorCalibratorBasic(IFSensor &sensor)
    : FilamentSensorCalibrator(sensor) {}

bool FilamentSensorCalibratorBasic::is_ready_for_calibration(CalibrationPhase phase) const {
    // The filament sensor does not require a calibration per se, so just check that the states are reported as expected
    const auto state = sensor_.get_state();
    switch (phase) {

    case CalibrationPhase::not_inserted:
        return state == FilamentSensorState::NoFilament;

    case CalibrationPhase::inserted:
        return state == FilamentSensorState::HasFilament;

    case CalibrationPhase::_cnt:
        break;
    }

    bsod_unreachable();
}

void FilamentSensorCalibratorBasic::calibrate(CalibrationPhase phase) {
    // Fail if the actual fsensor value is not what we expected
    fail_if(!is_ready_for_calibration(phase));
}

void FilamentSensorCalibratorBasic::finish() {
    // Nothing to do, nothing to store
}
