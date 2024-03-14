/**
 * @file MItem_MINI_MK3.5.cpp
 */

#include "MItem_MINI_MK3.5.hpp"
#include "img_resources.hpp"
#include "filament_sensors_handler.hpp"
#include "filament_sensor.hpp"

MI_FILAMENT_SENSOR_STATE::MI_FILAMENT_SENSOR_STATE()
    : WI_SWITCH_0_1_NA_t(get_state(), _(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

MI_FILAMENT_SENSOR_STATE::state_t MI_FILAMENT_SENSOR_STATE::get_state() {
    fsensor_t fs = FSensors_instance().GetPrimaryRunout();
    switch (fs) {
    case fsensor_t::HasFilament:
        return state_t::high;
    case fsensor_t::NoFilament:
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
