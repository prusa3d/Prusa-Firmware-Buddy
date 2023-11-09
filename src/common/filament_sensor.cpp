/**
 * @file filament_sensor.cpp
 * @author Radek Vana
 * @date 2019-12-16
 */

#include "filament_sensor.hpp"
#include "fsensor_eeprom.hpp"
#include "rtos_api.hpp"
#include "metric.h"

// delay between calls must be 1us or longer
void IFSensor::Cycle() {
    // sensor is disabled (only init can enable it)
    if (state == fsensor_t::Disabled) {
        record_state();
        return;
    }

    cycle();

    record_state();
}

/*---------------------------------------------------------------------------*/
// global thread safe functions
// but cannot be called from interrupt
void IFSensor::Enable() {
    CriticalSection C;
    enable();
    FSensorEEPROM::Set();
}

void IFSensor::Disable() {
    CriticalSection C;
    disable();
    FSensorEEPROM::Clr();
}

fsensor_t FSensor::WaitInitialized() {
    fsensor_t ret = FSensor::Get();
    while (ret == fsensor_t::NotInitialized) {
        Rtos::Delay(0); // switch to other threads
        ret = FSensor::Get();
    }
    return ret;
}

/*---------------------------------------------------------------------------*/
// global not thread safe functions
void FSensor::init() {
    bool enabled = FSensorEEPROM::Get(); // can globally disable all sensors, but some sensors might need another enable

    if (enabled) {
        enable();
    } else {
        disable();
    }
}

/*---------------------------------------------------------------------------*/
IFSensor::event IFSensor::GenerateEvent() {
    const auto previous_state = last_evaluated_state;
    last_evaluated_state = state;

    const bool has_filament = fsensor_t::HasFilament == state;

    // don't generate edges from not-working states or to not-working states
    if (((previous_state != fsensor_t::HasFilament) && (previous_state != fsensor_t::NoFilament))
        || ((last_evaluated_state != fsensor_t::HasFilament) && (last_evaluated_state != fsensor_t::NoFilament))) {
        return has_filament ? event::HasFilament : event::NoFilament;
    }

    const bool had_filament = fsensor_t::HasFilament == previous_state;
    if (has_filament == had_filament) {
        return has_filament ? event::HasFilament : event::NoFilament;
    }
    /// state has changed
    if (has_filament) {
        return event::EdgeFilamentInserted; // has && !had
    }
    return event::EdgeFilamentRemoved; //! has && had
}
