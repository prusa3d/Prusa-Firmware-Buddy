#include "filament_sensor_calibrator_invertible.hpp"

#include <option/has_fsensor_invertible.h>
#include <option/has_side_fsensor_invertible.h>

#include <config_store/store_instance.hpp>
#include <bsod/bsod.h>

FilamentSensorState FilamentSensorCalibratorInvertible::expected_state(CalibrationPhase phase, bool inverted) {
    switch (phase) {

    case CalibrationPhase::inserted:
        return FSensorInvertible::inverted_state(FilamentSensorState::HasFilament, inverted);

    case CalibrationPhase::not_inserted:
        return FSensorInvertible::inverted_state(FilamentSensorState::NoFilament, inverted);

    case CalibrationPhase::_cnt:
        break;
    }
    bsod_unreachable();
}

FilamentSensorCalibratorInvertible::FilamentSensorCalibratorInvertible(FSensorInvertible &sensor)
    : FilamentSensorCalibrator(sensor)
    , sensor_(sensor) {}

bool FilamentSensorCalibratorInvertible::is_ready_for_calibration(CalibrationPhase phase) const {
    const auto raw_state = sensor_.raw_state();

    return (is_ok_inverted_ && raw_state == expected_state(phase, true))
        || (is_ok_not_inverted_ && raw_state == expected_state(phase, false));
}

void FilamentSensorCalibratorInvertible::calibrate(CalibrationPhase phase) {
    const auto raw_state = sensor_.raw_state();

    is_ok_inverted_ &= (raw_state == expected_state(phase, true));
    is_ok_not_inverted_ &= (raw_state == expected_state(phase, false));

    fail_if(!is_ok_inverted_ && !is_ok_not_inverted_);
}

void FilamentSensorCalibratorInvertible::finish() {
    // Calling calibrate just once should set at least one of the is_oks to false
    fail_if(is_ok_inverted_ == is_ok_not_inverted_);

    const bool invert = !is_ok_not_inverted_;

    switch (sensor_.id().position) {

    case FilamentSensorID::Position::extruder:
#if HAS_FSENSOR_INVERTIBLE()
    #error Not implemented
#else
        bsod_unreachable();
#endif

    case FilamentSensorID::Position::side:
#if HAS_SIDE_FSENSOR_INVERTIBLE()
        config_store().selftest_result_side_fsensor.set(sensor_.id().index, failed() ? TestResult::failed : TestResult::passed);
        if (!failed()) {
            config_store().side_fsensor_polarity_inverted_bits.apply([&](auto &item) {
                item.set(sensor_.id().index, invert);
            });
        }
#else
        bsod_unreachable();
#endif
        break;
    }

    sensor_.load_settings();
}
