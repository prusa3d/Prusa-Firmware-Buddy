/**
 * @file filament_sensor.cpp
 * @author Radek Vana
 * @date 2019-12-16
 */

#include "filament_sensor.hpp"
#include "fsensor_eeprom.hpp"
#include "rtos_api.hpp"
#include "metric.h"

//delay between calls must be 1us or longer
std::optional<IFSensor::event> IFSensor::Cycle() {
    volatile const fsensor_t last_state_before_cycle = state;

    //sensor is disabled (only init can enable it)
    if (last_state_before_cycle == fsensor_t::Disabled) {
        record_state();
        return std::nullopt;
    }

    cycle();

    record_state();

    return generateEvent(last_state_before_cycle);
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
//but cannot be called from interrupt
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
//global not thread safe functions
void FSensor::init() {
    bool enabled = FSensorEEPROM::Get(); // can globally disable all sensors, but some sensors might need another enable

    if (enabled)
        enable();
    else
        disable();
}

/*---------------------------------------------------------------------------*/
//methods called only in fs_cycle
IFSensor::event IFSensor::generateEvent(fsensor_t previous_state) const {
    const bool has_filament = fsensor_t::HasFilament == state;

    // don't generate edges from not working states
    if ((previous_state != fsensor_t::HasFilament) && (previous_state != fsensor_t::NoFilament)) {
        return has_filament ? event::HasFilament : event::NoFilament;
    }

    const bool had_filament = fsensor_t::HasFilament == previous_state;
    if (has_filament == had_filament)
        return has_filament ? event::HasFilament : event::NoFilament;
    /// state has changed
    if (has_filament)
        return event::EdgeFilamentInserted; //has && !had
    return event::EdgeFilamentRemoved;      //!has && had
}
