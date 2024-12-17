/// @file
#pragma once

#include <cstdint>

namespace buddy {

/** Represents door sensor. */
class DoorSensor {
public:
    /** Represents state of the door sensor. */
    enum class State {
        sensor_detached,
        door_open,
        door_closed,
    };

    /** Return state of the door sensor. */
    State state() const { return detailed_state().state; }

    /** Represents detailed state of the door sensor, including raw sensor data. */
    struct DetailedState {
        State state;
        uint16_t raw_data;

        constexpr auto operator<=>(const DetailedState &) const = default;
    };

    /** Return detailed state of the door sensor. */
    DetailedState detailed_state() const;
};

/** Return global door sensor instance. */
DoorSensor &door_sensor();

} // namespace buddy
