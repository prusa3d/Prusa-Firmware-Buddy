/**
 * @file filament_sensor_mmu.cpp
 */

#include "filament_sensor_mmu.hpp"
#include "rtos_api.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"

using namespace MMU2;

void FSensorMMU::enable() {
}

void FSensorMMU::disable() {
}

void FSensorMMU::cycle() {
    // convert MMU state, not all states are used
    switch (mmu2.State()) {
    case xState::Active:
        state = mmu2.FindaDetectsFilament() ? fsensor_t::HasFilament : fsensor_t::NoFilament;
        break;
    case xState::Connecting:
        state = fsensor_t::NotInitialized;
        break;
    case xState::Stopped:
        state = fsensor_t::Disabled;
        break;
    }
}

void FSensorMMU::record_state() {
}
