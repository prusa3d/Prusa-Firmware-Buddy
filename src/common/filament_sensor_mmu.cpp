/**
 * @file filament_sensor_mmu.cpp
 */

#include "filament_sensor_mmu.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"

using namespace MMU2;

void FSensorMMU::cycle() {
    // convert MMU state, not all states are used
    switch (mmu2.State()) {
    case xState::Active:
        state = mmu2.FindaDetectsFilament() ? FilamentSensorState::HasFilament : FilamentSensorState::NoFilament;
        break;
    case xState::Connecting:
    case xState::Bootloader:
        state = FilamentSensorState::NotInitialized;
        break;
    case xState::Stopped:
        state = FilamentSensorState::Disabled;
        break;
    }
}
