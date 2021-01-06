// odometer.cpp

#include <cmath>

#include "odometer.hpp"
#include "cmath_ext.h"
#include "eeprom.h"

odometer_c odometer;
static const constexpr int E_AXIS = 3;

/// Saves all axes to EEPROM if any value has changed
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

/// Increments value of an axis
void odometer_c::add_new_value(int axis, float value) {
    /// E axis counts filament used instead of filament moved
    trip_xyze[axis] += (axis == E_AXIS) ? value : ABS(value);
}

/// Reads a value of the specific axis from EEPROM
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

/// \returns a value of the specific axis
float odometer_c::get(int axis) {
    if (axis < 0 || axis >= ODOMETER_AXES)
        return nanf("-");
    return get_from_eeprom(axis) + MAX(0, trip_xyze[axis]);
}
