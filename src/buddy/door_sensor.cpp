/// @file
#include <buddy/door_sensor.hpp>

#include <common/adc.hpp>
#include <option/has_door_sensor.h>
#include <printers.h>

static_assert(HAS_DOOR_SENSOR());

buddy::DoorSensor::DetailedState buddy::DoorSensor::detailed_state() const {
    // If this starts failing, check that the implementation of this function
    // is correct for the door sensor of the printer you are adding.
    // MK4: Checking door sensor presence (valid FW-HW combo check)
    static_assert(PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_MK4());

    // The 12-bit door sensor is returning approximate values of:
    //   0x000: Door closed
    //   0x7ff: Door open
    //   0xfff: Sensor detached
    // We will use midpoints between these values to discriminate states.
    const auto raw_data = AdcGet::door_sensor();
    if (raw_data < 0x3ff) {
        return { State::door_closed, raw_data };
    }
    if (raw_data < 0xcff) {
        return { State::door_open, raw_data };
    }
    return { State::sensor_detached, raw_data };
}

buddy::DoorSensor &buddy::door_sensor() {
    static buddy::DoorSensor instance;
    return instance;
}
