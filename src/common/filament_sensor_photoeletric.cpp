/**
 * @file filament_sensor_photoeletric.cpp
 * @author Radek Vana
 * @date 2021-02-12
 */

//there is 10kOhm PU to 5V in filament sensor
//MCU PU/PD is in range 30 - 50 kOhm

//when PD is selected and sensor is connected Vmcu = min 3.75V .. (5V * 30kOhm) / (30 + 10 kOhm)
//pin is 5V tolerant

//MCU has 5pF, transistor D-S max 15pF
//max R is 50kOhm
//Max Tau ~= 20*10^-12 * 50*10^3 = 1*10^-6 s ... about 1us

#include "filament_sensor_photoeletric.hpp"
#include "fsensor_pins.hpp"
#include "rtos_api.hpp"

static volatile fsensor_t last_state = fsensor_t::NotInitialized;

void FSensor::cycle0() {
    if (FSensorPins::Get()) {
        FSensorPins::pullDown();
        meas_cycle = 1; //next cycle shall be 1
    } else {
        set_state(fsensor_t::NoFilament); //it is filtered, 2 requests are needed to change state
        meas_cycle = 0;                   //remain in cycle 0
    }
}

void FSensor::cycle1() {
    //pulldown was set in cycle 0
    set_state(FSensorPins::Get() ? fsensor_t::HasFilament : fsensor_t::NotConnected);
    FSensorPins::pullUp();
    meas_cycle = 0; //next cycle shall be 0
}

void FSensor::cycle() {
    if (meas_cycle == 0) {
        cycle0();
    } else {
        cycle1();
    }
}

void FSensor::enable() {
    FSensorPins::pullUp();
    state = fsensor_t::NotInitialized;
    last_state = fsensor_t::NotInitialized;
    meas_cycle = 0;
}

void FSensor::disable() {
    state = fsensor_t::Disabled;
    last_state = fsensor_t::Disabled;
    meas_cycle = 0;
}

//not recorded
void FSensor::record_state() {
}

//simple filter
//without filter fs_meas_cycle1 could set FS_NO_SENSOR (in case filament just runout)
void FSensor::set_state(fsensor_t st) {
    CriticalSection C;
    if (last_state == st)
        state = st;
    last_state = st;
}
