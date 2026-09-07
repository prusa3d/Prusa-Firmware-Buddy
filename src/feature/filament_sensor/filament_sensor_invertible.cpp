/// @file
#include "filament_sensor_invertible.hpp"

#include <config_store/store_instance.hpp>
#include <test_result.hpp>
#include <option/has_fsensor_invertible.h>
#include <option/has_side_fsensor_invertible.h>
#include <feature/filament_sensor/calibrator/filament_sensor_calibrator_invertible.hpp>
#include <bsod/bsod.h>

FilamentSensorState FSensorInvertible::inverted_state(FilamentSensorState state, bool inverted) {
    if (!inverted) {
        return state;
    }

    switch (state) {

    case FilamentSensorState::HasFilament:
        return FilamentSensorState::NoFilament;

    case FilamentSensorState::NoFilament:
        return FilamentSensorState::HasFilament;

    default:
        return state;
    }
}

FSensorInvertible::FSensorInvertible(FilamentSensorID id)
    : IFSensor(id) {
    load_settings();
}

void FSensorInvertible::load_settings() {
    switch (id().position) {

    case FilamentSensorID::Position::extruder:
#if HAS_FSENSOR_INVERTIBLE()
    #error Not implemented
#else
        bsod_unreachable();
#endif

    case FilamentSensorID::Position::side:
#if HAS_SIDE_FSENSOR_INVERTIBLE()
        is_calibrated_ = (config_store().selftest_result_side_fsensor.get(id_.index) == TestResult::passed);
        is_inverted_ = config_store().side_fsensor_polarity_inverted_bits.get()[id_.index];
#else
        bsod_unreachable();
#endif
        break;
    }
}

void FSensorInvertible::cycle() {
    if (raw_state_.load() == FilamentSensorState::NotConnected) {
        // A disconnected sensor cannot be calibrated nor read; not-connected takes priority over not-calibrated.
        state = FilamentSensorState::NotConnected;
    } else if (!is_calibrated_) {
        state = FilamentSensorState::NotCalibrated;
    } else {
        state = inverted_state(raw_state_, is_inverted_);
    }
}

FilamentSensorCalibrator *FSensorInvertible::create_calibrator(FilamentSensorCalibrator::Storage &storage) {
    return &storage.emplace<FilamentSensorCalibratorInvertible>(*this);
}
