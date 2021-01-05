// odometer.cpp

#include <cmath>

#include "odometer.hpp"
#include "cmath_ext.h"
#include "eeprom.h"

odometer_c odometer;
/// minimal value saved to EEPROM
static const constexpr float min_trip = 10000;
static const constexpr int E_AXIS = 3;

void odometer_c::lazy_add_to_eeprom(int axis) {
    bool save = false;
    if (axis >= 0) {
        save = save || trip_xyze[axis] >= min_trip;
    } else {
        for (int i = 0; i < ODOMETER_AXES; i++)
            save = save || trip_xyze[i] >= min_trip;
    }
    if (!save)
        return;

    float odo_xyze[ODOMETER_AXES];
    /// TODO: read EEPROM to odo_xyze
    for (int i = 0; i < ODOMETER_AXES; i++) {
        if (trip_xyze[i] >= min_trip) {
            odo_xyze[i] += trip_xyze[i];
            trip_xyze[i] = 0;
        }
    }
    force_to_eeprom();
}

void odometer_c::force_to_eeprom() {
    bool changed = false;
    for (int i = 0; i < ODOMETER_AXES; ++i) {
        if (trip_xyze[i] != 0) {
            changed = true;
            break;
        }
    }
    if (!changed)
        return;

    eeprom_set_var(EEVAR_ODOMETER_X, variant8_flt(get(0)));
    eeprom_set_var(EEVAR_ODOMETER_Y, variant8_flt(get(1)));
    eeprom_set_var(EEVAR_ODOMETER_Z, variant8_flt(get(2)));
    eeprom_set_var(EEVAR_ODOMETER_E, variant8_flt(get(3)));
    for (int i = 0; i < ODOMETER_AXES; ++i)
        trip_xyze[i] = 0;
}

void odometer_c::add_new_value(int axis, float value) {
    /// E axis counts filament used instead of filament moved
    trip_xyze[axis] += (axis == E_AXIS) ? value : ABS(value);
    lazy_add_to_eeprom(axis);
}

float odometer_c::get_from_eeprom(int axis) {
    switch (axis) {
    case 0:
        return variant8_get_flt(eeprom_get_var(EEVAR_ODOMETER_X));
    case 1:
        return variant8_get_flt(eeprom_get_var(EEVAR_ODOMETER_Y));
    case 2:
        return variant8_get_flt(eeprom_get_var(EEVAR_ODOMETER_Z));
    case 3:
        return variant8_get_flt(eeprom_get_var(EEVAR_ODOMETER_E));
    }
    return nanf("-");
}

float odometer_c::get(int axis) {
    if (axis < 0 || axis >= ODOMETER_AXES)
        return nanf("-");
    return get_from_eeprom(axis) + trip_xyze[axis];
}
