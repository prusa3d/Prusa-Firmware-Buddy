// odometer_s.cpp

#include <cmath>
#include "odometer.hpp"
#include <option/has_wastebin_fill_tracking.h>
#include "cmath_ext.h"
#include <config_store/store_instance.hpp>
#include <bsod/bsod.h>

Odometer_s Odometer_s::instance_;

bool Odometer_s::changed() {
    // Note: While running the force_to_eeprom, it's possible a get will
    // temporarily get slightly wrong value. Next time it'll be correct.
    for (size_t i = 0; i < axis_count; ++i) {
        if (trip_xyz[i] != 0) {
            return true;
        }
    }

    for (const auto tool : PhysicalToolIndex::all()) {
        if (extruded[tool] != 0) {
            return true;
        }
        if (toolpick[tool] != 0) {
            return true;
        }
    }

    if (duration_time != 0) {
        return true;
    }

    if (mmu_changes != 0) {
        return true;
    }

#if HAS_WASTEBIN_FILL_TRACKING()
    if (nozzle_cleaner_pellets != 0) {
        return true;
    }
#endif

    return false;
}

void Odometer_s::force_to_eeprom() {
    if (!changed()) {
        return;
    }

    auto &store = config_store();
    auto transaction = store.get_backend().transaction_guard();

    for (size_t i = 0; i < axis_count; ++i) {
        store.set_odometer_axis(i, get_axis(axis_t(i)));
        trip_xyz[i] = 0;
    }

    for (const auto tool : PhysicalToolIndex::all()) {
        store.set_odometer_extruded_length(tool, get_extruded(tool));
        extruded[tool] = 0;

        store.set_odometer_toolpicks(tool, get_toolpick(tool));
        toolpick[tool] = 0;
    }

    store.odometer_time.set(get_time());
    duration_time = 0;

    store.mmu_changes.set(get_mmu_changes());
    mmu_changes = 0;

#if HAS_WASTEBIN_FILL_TRACKING()
    store.nozzle_cleaner_pellets.set(get_nozzle_cleaner_pellets());
    nozzle_cleaner_pellets = 0;
#endif
}

void Odometer_s::add_axis(axis_t axis, float value) {
    // Technically, this is only weakly thread safe. Running this function from
    // multiple threads will not cause UB, but could still lose one of the
    // updates.
    //
    // This is not a problem, as the updates and storing is called only from
    // marlin. We need the atomics mostly to read them at the same time, while
    // printing.
    debug_assert(axis < axis_t::count_);
    trip_xyz[std::to_underlying(axis)] += std::abs(value);
}

float Odometer_s::get_axis(axis_t axis) {
    debug_assert(axis <= axis_t::count_);
    return config_store().get_odometer_axis(std::to_underlying(axis)) + trip_xyz[std::to_underlying(axis)].load();
}

void Odometer_s::add_extruded(PhysicalToolIndex tool, float value) {
    extruded[tool] += value; // E axis counts filament used instead of filament moved
}

float Odometer_s::get_extruded(PhysicalToolIndex tool) {
    return config_store().get_odometer_extruded_length(tool) + MAX(0.0f, extruded[tool].load());
}

float Odometer_s::get_extruded_all() {
    float sum = 0;
    for (const auto tool : PhysicalToolIndex::all()) {
        sum += get_extruded(tool);
    }
    return sum;
}

void Odometer_s::add_toolpick(PhysicalToolIndex tool) {
    toolpick[tool]++;
}

uint32_t Odometer_s::get_toolpick(PhysicalToolIndex tool) {
    return config_store().get_odometer_toolpicks(tool) + toolpick[tool].load();
}

uint32_t Odometer_s::get_toolpick_all() {
    uint32_t sum = 0;
    for (const auto tool : PhysicalToolIndex::all()) {
        sum += get_toolpick(tool);
    }
    return sum;
}

void Odometer_s::add_mmu_change() {
    mmu_changes++;
}

uint32_t Odometer_s::get_mmu_changes() {
    return config_store().mmu_changes.get() + mmu_changes;
}

#if HAS_WASTEBIN_FILL_TRACKING()
void Odometer_s::add_nozzle_cleaner_pellet() {
    nozzle_cleaner_pellets++;
}

uint32_t Odometer_s::get_nozzle_cleaner_pellets() {
    return config_store().nozzle_cleaner_pellets.get() + nozzle_cleaner_pellets;
}

void Odometer_s::reset_nozzle_cleaner_pellets() {
    // get_nozzle_cleaner_pellets() returns persisted + RAM accumulator, so zero both.
    nozzle_cleaner_pellets = 0;
    config_store().nozzle_cleaner_pellets.set(0);
}
#endif

void Odometer_s::add_time(uint32_t value) {
    duration_time += value;
}

uint32_t Odometer_s::get_time() {
    uint32_t time = config_store().odometer_time.get() + MAX(0ul, duration_time.load());
    return time;
}
