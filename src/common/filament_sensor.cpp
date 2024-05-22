/**
 * @file filament_sensor.cpp
 * @author Radek Vana
 * @date 2019-12-16
 */

#include "filament_sensor.hpp"
#include "rtos_api.hpp"
#include "metric.h"

void IFSensor::check_for_events() {
    const auto previous_state = last_check_event_state_;
    last_check_event_state_ = state;
    last_event_ = Event::no_event;

    // Generate edge events only if we go from one working state to another (HasFilament <-> NoFilament)
    if (!is_fsensor_working_state(state) || !is_fsensor_working_state(previous_state)) {
        return;
    }

    if (state == previous_state) {
        return;
    }

    last_event_ = (state == FilamentSensorState::HasFilament) ? Event::filament_inserted : Event::filament_removed;
}

void IFSensor::force_set_enabled(bool set) {
    state = set ? FilamentSensorState::NotInitialized : FilamentSensorState::Disabled;
}
