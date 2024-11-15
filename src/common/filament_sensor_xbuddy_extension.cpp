#include "filament_sensor_xbuddy_extension.hpp"

#include <feature/xbuddy_extension/xbuddy_extension.hpp>

using namespace buddy;

void FSensorXBuddyExtension::cycle() {
    state = interpret_state();
}

int32_t FSensorXBuddyExtension::GetFilteredValue() const {
    return xbuddy_extension().filament_sensor().transform([](auto v) { return static_cast<int>(v); }).value_or(-1);
}

FilamentSensorState FSensorXBuddyExtension::interpret_state() const {
    switch (xbuddy_extension().status()) {

    case XBuddyExtension::Status::disabled:
        return FilamentSensorState::Disabled;

    case XBuddyExtension::Status::not_connected:
        return FilamentSensorState::NotConnected;

    case XBuddyExtension::Status::ready:
        // Continue
        break;
    }

    switch (xbuddy_extension().filament_sensor().value_or(XBuddyExtension::FilamentSensorState::uninitialized)) {

    case XBuddyExtension::FilamentSensorState::disconnected:
        return FilamentSensorState::NotConnected;

    case XBuddyExtension::FilamentSensorState::uninitialized:
        return FilamentSensorState::NotInitialized;

    case XBuddyExtension::FilamentSensorState::has_filament:
        return FilamentSensorState::HasFilament;

    case XBuddyExtension::FilamentSensorState::no_filament:
        return FilamentSensorState::NoFilament;
    }

    return FilamentSensorState::NotInitialized;
}
