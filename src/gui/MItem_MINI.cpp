/**
 * @file MItem_MINI.cpp
 */

#include "MItem_MINI.hpp"
#include "img_resources.hpp"
#include "filament_sensors_handler.hpp"
#include "filament_sensor.hpp"

MI_FILAMENT_SENSOR_STATE::MI_FILAMENT_SENSOR_STATE()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_FILAMENT_SENSOR_STATE::state_t MI_FILAMENT_SENSOR_STATE::get_state() {
    FilamentSensorState fs = FSensors_instance().sensor_state(LogicalFilamentSensor::primary_runout);
    switch (fs) {
    case FilamentSensorState::HasFilament:
        return state_t::high;
    case FilamentSensorState::NoFilament:
        return state_t::low;
    default:;
    }
    return state_t::unknown;
}

void MI_FILAMENT_SENSOR_STATE::Loop() {
    SetIndex((size_t)get_state());
}

MI_MINDA::MI_MINDA()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_MINDA::state_t MI_MINDA::get_state() {
    return (buddy::hw::zMin.read() == buddy::hw::Pin::State::low) ? state_t::low : state_t::high;
}

void MI_MINDA::Loop() {
    SetIndex((size_t)get_state());
}
