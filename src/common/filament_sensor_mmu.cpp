/**
 * @file filament_sensor_mmu.cpp
 */

#include "filament_sensor_mmu.hpp"
#include "rtos_api.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2mk404.h"

using namespace MMU2;

void FSensorMMU::enable() {
}

void FSensorMMU::disable() {
}

void FSensorMMU::cycle() {
    // convert MMU state, not all states are used
    switch (mmu2.State()) {
    case State_t::Active:
        state = mmu2.FindaDetectsFilament() ? fsensor_t::HasFilament : fsensor_t::NoFilament;
        break;
    case State_t::Connecting:
        state = fsensor_t::NotInitialized;
        break;
    case State_t::Stopped:
        state = fsensor_t::Disabled;
        break;
    }
}

void FSensorMMU::record_state() {
}
