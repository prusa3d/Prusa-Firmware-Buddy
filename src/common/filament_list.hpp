#pragma once

#include <variant>

#include <inplace_vector.hpp>
#include <tool_index.hpp>

#include "filament.hpp"

using FilamentList = stdext::inplace_vector<FilamentType, total_filament_type_count>;

/// List of all filaments, in the order that makes kind of sense to the user
/// !!! Order of items in the list can change between builds. Do not rely on it.
extern constinit const FilamentList all_filament_types;

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

    /// Restricts the output to filaments compatible with the given virtual tool's hotend.
    /// AllTools: the filament must be compatible with every enabled virtual tool's hotend.
    /// NoTool (default): no compatibility filter.
    /// \p enforce_first_item bypasses this filter (kept at position 0 regardless of compatibility).
    using ToolFilter = std::variant<VirtualToolIndex, AllTools, NoTool>;
    ToolFilter compatible_with_tool = NoTool {};
};

/// Generate filament list config for management purposes - show all, respect user ordering
extern const GenerateFilamentListConfig management_generate_filament_list_config;

/// Generates a filament list based on the provided \p config.
/// The result is stored in \p list. (But some slots might be unused).
void generate_filament_list(FilamentList &list, const GenerateFilamentListConfig &config);
