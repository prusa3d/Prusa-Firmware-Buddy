// odometer_s.cpp

#include <cmath>

#include "odometer.hpp"
#include "cmath_ext.h"
#include "eeprom.h"

//translation table to get eevar from axis index
static constexpr int eevars[] = {
    EEVAR_ODOMETER_X,
    EEVAR_ODOMETER_Y,
    EEVAR_ODOMETER_Z,
    EEVAR_ODOMETER_E0
};

static_assert(sizeof(eevars) / sizeof(eevars[0]) == Odometer_s::axis_count, "count of axis does not match eeprom");

void Odometer_s::force_to_eeprom() {
    bool changed = false;
    for (size_t i = 0; i < axis_count; ++i) {
        if (trip_xyze[i] != 0) {
            changed = true;
            break;
        }
    }
    if (!changed)
        return;

    // cast is safe axis_count == axis_t::count_
    for (size_t i = 0; i < axis_count; ++i) {
        eeprom_set_var(eevars[i], variant8_flt(get(axis_t(i))));
        trip_xyze[i] = 0;
    }
}

void Odometer_s::add_value(int axis, float value) {
    /// E axis counts filament used instead of filament moved
    trip_xyze[axis] += (axis == int(axis_t::E)) ? value : ABS(value);
}

float Odometer_s::get_from_eeprom(axis_t axis) {
    return variant8_get_flt(eeprom_get_var(eevars[size_t(axis)]));
}

float Odometer_s::get(axis_t axis) {
    return get_from_eeprom(axis) + MAX(0, trip_xyze[size_t(axis)]);
}
