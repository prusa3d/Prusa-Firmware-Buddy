#include "filament_sensor_ix_side.hpp"

void FSensor_iXSide::cycle() {
    if (buddy::hw::backFilamentSensorDetect.read() == buddy::hw::Pin::State::high) {
        // PA8: Sensor connected detection (high = not connected [thanks to the pullup on the Buddy board], low = connected)
        state = FilamentSensorState::NotConnected;
    } else if (buddy::hw::backFilamentSensorState.read() == buddy::hw::Pin::State::high) {
        // PC9: State (high = filament detected, low = filament not detected)
        state = FilamentSensorState::HasFilament;
    } else {
        state = FilamentSensorState::NoFilament;
    }
}
