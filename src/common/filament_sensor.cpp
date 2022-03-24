/**
 * @file filament_sensor.cpp
 * @author Radek Vana
 * @date 2019-12-16
 */

#include "filament_sensor.hpp"
#include "fsensor_eeprom.hpp"
#include "rtos_api.hpp"
#include "metric.h"

FSensor::FSensor()
    : state(fsensor_t::NotInitialized)
    , meas_cycle(0) {
    init();
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
fsensor_t FSensor::Get() {
    return state;
}
/*---------------------------------------------------------------------------*/
//global thread safe functions
//but cannot be called from interrupt
void FSensor::Enable() {
    CriticalSection C;
    enable();
    FSensorEEPROM::Set();
}

void FSensor::Disable() {
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
    bool enabled = FSensorEEPROM::Get();

    if (enabled)
        enable();
    else
        disable();
}

/*---------------------------------------------------------------------------*/
//methods called only in fs_cycle
FSensor::event FSensor::generateEvent(fsensor_t previous_state) const {
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

//delay between calls must be 1us or longer
std::optional<FSensor::event> FSensor::Cycle() {
    volatile const fsensor_t last_state_before_cycle = state;

    //sensor is disabled (only init can enable it)
    if (last_state_before_cycle == fsensor_t::Disabled) {
        return std::nullopt;
    }

    cycle();
    record_state();

    return generateEvent(last_state_before_cycle);
}
