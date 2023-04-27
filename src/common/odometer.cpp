// odometer_s.cpp

#include <cmath>
#include "odometer.hpp"
#include "cmath_ext.h"

// translation table to get eevar from axis index
static constexpr enum eevar_id eevars_axis[] = {
    EEVAR_ODOMETER_X,
    EEVAR_ODOMETER_Y,
    EEVAR_ODOMETER_Z,
};

static constexpr enum eevar_id eevars_extruded[] = {
    EEVAR_ODOMETER_E0,
    EEVAR_ODOMETER_E1,
    EEVAR_ODOMETER_E2,
    EEVAR_ODOMETER_E3,
    EEVAR_ODOMETER_E4,
    EEVAR_ODOMETER_E5,
};

static constexpr enum eevar_id eevars_toolpick[] = {
    EEVAR_ODOMETER_T0,
    EEVAR_ODOMETER_T1,
    EEVAR_ODOMETER_T2,
    EEVAR_ODOMETER_T3,
    EEVAR_ODOMETER_T4,
    EEVAR_ODOMETER_T5,
};

static_assert(std::size(eevars_axis) == Odometer_s::axis_count, "Count of axes does not match eeprom");
static_assert(std::size(eevars_extruded) == EEPROM_MAX_TOOL_COUNT, "Count of extruders does not match eeprom");
static_assert(std::size(eevars_toolpick) == EEPROM_MAX_TOOL_COUNT, "Count of tools does not match eeprom");
static_assert(HOTENDS <= EEPROM_MAX_TOOL_COUNT, "Too many hotends");

bool Odometer_s::changed() {
    // Note: While running the force_to_eeprom, it's possible a get will
    // temporarily get slightly wrong value. Next time it'll be correct.
    for (size_t i = 0; i < axis_count; ++i) {
        if (trip_xyz[i] != 0) {
            return true;
        }
    }

    for (size_t i = 0; i < HOTENDS; ++i) {
        if (extruded[i] != 0) {
            return true;
        }
        if (toolpick[i] != 0) {
            return true;
        }
    }

    if (duration_time != 0) {
        return true;
    }

    return false;
}

void Odometer_s::force_to_eeprom() {
    if (!changed()) {
        return;
    }

    for (size_t i = 0; i < axis_count; ++i) {
        eeprom_set_flt(eevars_axis[i], get_axis(axis_t(i)));
        trip_xyz[i] = 0;
    }

    for (size_t i = 0; i < HOTENDS; ++i) {
        eeprom_set_flt(eevars_extruded[i], get_extruded(i));
        extruded[i] = 0;
    }

    for (size_t i = 0; i < HOTENDS; ++i) {
        eeprom_set_ui32(eevars_toolpick[i], get_toolpick(i));
        toolpick[i] = 0;
    }

    eeprom_set_ui32(EEVAR_ODOMETER_TIME, get_time());
    duration_time = 0;
}

void Odometer_s::add_axis(axis_t axis, float value) {
    // Technically, this is only weakly thread safe. Running this function from
    // multiple threads will not cause UB, but could still lose one of the
    // updates.
    //
    // This is not a problem, as the updates and storing is called only from
    // marlin. We need the atomics mostly to read them at the same time, while
    // printing.
    assert(axis < axis_t::count_);
    trip_xyz[ftrstd::to_underlying(axis)] += std::abs(value);
}

float Odometer_s::get_axis(axis_t axis) {
    assert(axis <= axis_t::count_);
    return eeprom_get_flt(eevars_axis[ftrstd::to_underlying(axis)]) + trip_xyz[ftrstd::to_underlying(axis)].load();
}

void Odometer_s::add_extruded(uint8_t extruder, float value) {
    assert(extruder < HOTENDS);
    extruded[extruder] += value; // E axis counts filament used instead of filament moved
}

float Odometer_s::get_extruded(uint8_t extruder) {
    assert(extruder < HOTENDS);
    return eeprom_get_flt(eevars_extruded[extruder]) + MAX(0.0f, extruded[extruder].load());
}

float Odometer_s::get_extruded_all() {
    float sum = 0;
    for (uint8_t i = 0; i < HOTENDS; ++i) {
        sum += get_extruded(i);
    }
    return sum;
}

void Odometer_s::add_toolpick(uint8_t extruder) {
    assert(extruder < HOTENDS);
    toolpick[extruder]++;
}

uint32_t Odometer_s::get_toolpick(uint8_t extruder) {
    assert(extruder < HOTENDS);
    return eeprom_get_ui32(eevars_toolpick[extruder]) + toolpick[extruder].load();
}

uint32_t Odometer_s::get_toolpick_all() {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < HOTENDS; ++i) {
        sum += get_toolpick(i);
    }
    return sum;
}

void Odometer_s::add_time(uint32_t value) {
    duration_time += value;
}

uint32_t Odometer_s::get_time() {
    uint32_t time = eeprom_get_ui32(EEVAR_ODOMETER_TIME) + MAX(0ul, duration_time.load());
    return time;
}
