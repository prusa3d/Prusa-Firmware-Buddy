/**
 * @file filament_sensor.cpp
 * @author Radek Vana
 * @date 2019-12-16
 */

#include "filament_sensor.hpp"
#include "fsensor_eeprom.hpp"
#include "rtos_api.hpp"
#include "metric.h"

void IFSensor::init() {
    set_enabled(FSensorEEPROM::Get());
}

IFSensor::Event IFSensor::generate_event() {
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

void IFSensor::set_enabled(bool set) {
    state = set ? FilamentSensorState::NotInitialized : FilamentSensorState::Disabled;
}
