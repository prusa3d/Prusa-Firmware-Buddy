// odometer.cpp

#include "odometer.hpp"
#include "cmath_ext.h"

odometer_c odometer;
/// minimal value saved to EEPROM
static const constexpr float min_trip = 10000;
static const constexpr int E_AXIS = 3;

void odometer_c::lazy_add_to_eeprom(int axis) {
    bool save = false;
    if (axis >= 0) {
        save = save || trip_xyze[axis] >= min_trip;
    } else {
        for (int i = 0; i < 4; i++)
            save = save || trip_xyze[i] >= min_trip;
    }
    if (!save)
        return;

    float odo_xyze[4];
    /// TODO: read EEPROM to odo_xyze
    for (int i = 0; i < 4; i++) {
        if (trip_xyze[i] >= min_trip) {
            odo_xyze[i] += trip_xyze[i];
            trip_xyze[i] = 0;
        }
    }
    force_to_eeprom();
}

void odometer_c::force_to_eeprom() {
    /// TODO:
}

void odometer_c::add_new_value(int axis, float value) {
    /// E axis counts filament used instead of filament moved
    trip_xyze[axis] += (axis == E_AXIS) ? value : ABS(value);
    lazy_add_to_eeprom(axis);
}
