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
#include "fsensor_pins.hpp"
#include "rtos_api.hpp"

void FSensorPhotoElectric::cycle0() {
    if (FSensorPins::Get()) {
        FSensorPins::pullDown();
        measure_cycle = Cycle::no1; // next cycle shall be 1
    } else {
        set_state(fsensor_t::NoFilament); // it is filtered, 2 requests are needed to change state
        measure_cycle = Cycle::no0; // remain in cycle 0
    }
}

void FSensorPhotoElectric::cycle1() {
    // pulldown was set in cycle 0
    set_state(FSensorPins::Get() ? fsensor_t::HasFilament : fsensor_t::NotConnected);
    FSensorPins::pullUp();
    measure_cycle = Cycle::no0; // next cycle shall be 0
}

void FSensorPhotoElectric::cycle() {
    switch (measure_cycle) {
    case Cycle::no0:
        cycle0();
        return;
    case Cycle::no1:
        cycle1();
        return;
    }
}

void FSensorPhotoElectric::enable() {
    FSensorPins::pullUp();
    state = fsensor_t::NotInitialized;
    last_state = fsensor_t::NotInitialized;
    measure_cycle = Cycle::no0;
}

void FSensorPhotoElectric::disable() {
    state = fsensor_t::Disabled;
    last_state = fsensor_t::Disabled;
    measure_cycle = Cycle::no0;
}

// not recorded
void FSensorPhotoElectric::record_state() {
}

// simple filter
// without filter fs_meas_cycle1 could set FS_NO_SENSOR (in case filament just runout)
void FSensorPhotoElectric::set_state(fsensor_t st) {
    CriticalSection C;
    if (last_state == st) {
        state = st;
    }
    last_state = st;
}

FSensorPhotoElectric::FSensorPhotoElectric() {
    init();
}
