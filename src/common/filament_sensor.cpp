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
    if (state == FilamentSensorState::Disabled) {
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
    FSensorEEPROM::Set();

    CriticalSection C;
    enable();
}

void IFSensor::Disable() {
    FSensorEEPROM::Clr();

    CriticalSection C;
    disable();
}

/*---------------------------------------------------------------------------*/
// global not thread safe functions
void IFSensor::init() {
    bool enabled = FSensorEEPROM::Get(); // can globally disable all sensors, but some sensors might need another enable

    if (enabled) {
        enable();
    } else {
        disable();
    }
}

/*---------------------------------------------------------------------------*/
IFSensor::Event IFSensor::GenerateEvent() {
    const auto previous_state = last_evaluated_state;
    last_evaluated_state = state;

    // Generate edge events only if we go from one working state to another (HasFilament <-> NoFilament)
    if (!is_fsensor_working_state(state) || !is_fsensor_working_state(previous_state)) {
        return Event::no_event;
    }

    if (state == previous_state) {
        return Event::no_event;
    }

    return (state == FilamentSensorState::HasFilament) ? Event::filament_inserted : Event::filament_removed;
}
