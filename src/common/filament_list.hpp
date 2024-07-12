#pragma once

#include "filament.hpp"

using FilamentListStorage = std::array<FilamentType, total_filament_type_count>;

/// List of all filaments, in the order that makes kind of sense to the user
/// !!! Order of items in the list can change between builds. Do not rely on it.
static constexpr FilamentListStorage all_filament_types = [] {
    FilamentListStorage r;

    size_t index = 0;

    // Preset filaments first
    for (size_t i = 0; i < static_cast<size_t>(PresetFilamentType::_count); i++) {
        r[index++] = static_cast<PresetFilamentType>(i);
    }

    if (index != r.size()) {
        std::abort();
    }

    return r;
}();
