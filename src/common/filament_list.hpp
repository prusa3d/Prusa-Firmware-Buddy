#pragma once

#include <array>

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

    for (uint8_t i = 0; i < user_filament_type_count; i++) {
        r[index++] = UserFilamentType { i };
    }

    if (index != r.size()) {
        std::abort();
    }

    return r;
}();

struct GenerateFilamentListConfig {
    /// If set, only outputs visible filaments
    bool visible_only = true;

    /// If set, visible items will be at the front
    bool visible_first = false;

    /// If set, the filaments will be sorted based on config_store().filament_order
    /// \p visible_first has precedence
    bool user_ordering = true;

    /// If set, the set filament type will be at the first position of the list, circumventing all filters and sorting rules
    FilamentType enforce_first_item = FilamentType::none;
};

/// Generate filament list config for management purposes - show all, respect user ordering
extern const GenerateFilamentListConfig management_generate_filament_list_config;

/// Generates a filament list based on the provided \p config.
/// The result is stored in \p storage. (But some slots might be unused).
/// \returns generated list size
size_t generate_filament_list(FilamentListStorage &storage, const GenerateFilamentListConfig &config);
