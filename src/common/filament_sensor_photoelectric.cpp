/**
 * @file filament_sensor_photoelectric.cpp
 * @author Radek Vana
 * @date 2021-02-12
 */

// there is 10kOhm PU to 5V in filament sensor
// MCU PU/PD is in range 30 - 50 kOhm

// when PD is selected and sensor is connected Vmcu = min 3.75V .. (5V * 30kOhm) / (30 + 10 kOhm)
// pin is 5V tolerant

// MCU has 5pF, transistor D-S max 15pF
// max R is 50kOhm
// Max Tau ~= 20*10^-12 * 50*10^3 = 1*10^-6 s ... about 1us

#include "filament_sensor_photoelectric.hpp"

#include <device/board.h>

void FSensorPhotoElectric::cycle() {
    // There are two printers that are using the FINDA - MK3.5 & MINI
    // MK3.5's sensor apparently has the logic inverted compared to MINI
    const bool pin_readout = buddy::hw::fSensor.read() == (BOARD_IS_XBUDDY() ? buddy::hw::Pin::State::low : buddy::hw::Pin::State::high);

    switch (measure_phase) {

        // Phase 0 - with pullUp
    case MeasurePhase::p0:
        if (pin_readout) {
            buddy::hw::fSensor.pullDown();
            measure_phase = MeasurePhase::p1;
        } else {
            set_state(FilamentSensorState::NoFilament); // it is filtered, 2 requests are needed to change state
            // remain in phase 0
        }
        break;

        // Phase 1 - with pullDown
    case MeasurePhase::p1:
        set_state(pin_readout ? FilamentSensorState::HasFilament : FilamentSensorState::NotConnected);
        buddy::hw::fSensor.pullUp();
        measure_phase = MeasurePhase::p0;
        break;
    }
}

void FSensorPhotoElectric::force_set_enabled(bool set) {
    IFSensor::force_set_enabled(set);

    if (set) {
        buddy::hw::fSensor.pullUp();
    }

    last_set_state_target = state;
    measure_phase = MeasurePhase::p0;
}

// simple filter - the same value needs to be written twice to take effect
// without filter fs_meas_cycle1 could set FS_NO_SENSOR (in case filament just runout)
void FSensorPhotoElectric::set_state(FilamentSensorState set) {
    if (last_set_state_target == set) {
        state = set;
    }
    last_set_state_target = set;
}
