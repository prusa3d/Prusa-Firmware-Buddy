#include "filament_list.hpp"
#include "encoded_filament.hpp"
#include <bsod/bsod.h>
#include <utils/bitset_utils.hpp>

#ifndef UNITTESTS
    // Used by generate_filament_list() below, which is itself UNITTESTS-excluded.
    #include <tool/physical_tool.hpp>
    #include <feature/compatibility_checks/filament_compatibility.hpp>
#endif

constinit const FilamentList all_filament_types = [] {
    FilamentList r;

    // Preset filaments first
    for (size_t i = 0; i < static_cast<size_t>(PresetFilamentType::_count); i++) {
        r.push_back(static_cast<PresetFilamentType>(i));
    }

    for (uint8_t i = 0; i < user_filament_type_count; i++) {
        r.push_back(UserFilamentType { i });
    }

    if (r.size() != total_filament_type_count) {
        std::abort();
    }

    return r;
}();

const GenerateFilamentListConfig management_generate_filament_list_config {
    .visible_only = false,
    .visible_first = false,
    .user_ordering = true,
};

#ifndef UNITTESTS
void generate_filament_list(FilamentList &list, const GenerateFilamentListConfig &config) {
    list.clear();

    std::bitset<256> is_filament_visible_bitset;
    static_assert(std::is_same_v<decltype(EncodedFilamentType::data), uint8_t>);

    // Generate visible list to prevent locking the config_store mutex many times
    if (config.visible_first || config.visible_only) {
        // If this changes, the generator code probably also needs to change
        static_assert(std::is_same_v<FilamentType_, std::variant<NoFilamentType, PresetFilamentType, UserFilamentType, AdHocFilamentType, PendingAdHocFilamentType>>);

        const auto is_preset_filament_visible = config_store().visible_preset_filament_types.get();
        for (size_t i = 0; i < static_cast<size_t>(PresetFilamentType::_count); i++) {
            const auto ft = static_cast<PresetFilamentType>(i);
            is_filament_visible_bitset.set(EncodedFilamentType(ft).data, is_preset_filament_visible.test(i));
        }

        const auto is_user_filament_visible = config_store().visible_user_filament_types.get();
        for (UserFilamentType ft; ft.index < user_filament_type_count; ft.index++) {
            is_filament_visible_bitset.set(EncodedFilamentType(ft).data, is_user_filament_visible.test(ft.index));
        }

        // Hide incompatible filaments
        for (FilamentType ft : all_filament_types) {
            const auto ix = EncodedFilamentType(ft).data;

            if (!is_filament_visible_bitset.test(ix)) {
                // Skip slow compatibility check
                continue;
            }

            const buddy::filament_compatibility::CompatibilityReportGenerateArgs args {
                .filament = ft.parameters(),
                .tools = config.compatible_with_tool,
                // This one is not worth plumbing through
                // It only extends the cases where filament is hidden by default
                // "Show all" will still make it selectable (and you will get an explanatory error when trying to select)
                .assume_filament_already_inserted = false,
            };
            buddy::filament_compatibility::CompatibilityReport report;
            report.generate_noclear(args);
            if (report.compatibility_level() >= buddy::compatibility_checks::CompatibilityLevel::fatal_incompatibility) {
                is_filament_visible_bitset.reset(ix);
            }
        }
    }

    const auto is_filament_visible = [&](FilamentType ft) {
        return is_filament_visible_bitset.test(EncodedFilamentType(ft).data);
    };

    std::bitset<256> is_filament_in_list_bitset;

    /// Appends filament to the list, if it is not already there and it is compatible with the target tool(s).
    /// NoTool (the default filter) accepts everything; a single virtual tool must support it;
    /// AllTools requires every enabled virtual tool's hotend to support it.
    const auto append_filament = [&](FilamentType ft) {
        const uint8_t ix = EncodedFilamentType(ft).data;
        if (is_filament_in_list_bitset.test(ix)) {
            return;
        }

        list.push_back(ft);
        is_filament_in_list_bitset.set(ix);
    };

    /// Walks filaments, one possibly multiple times
    const auto walk_filaments = [&](auto &&f) {
        // First walk user ordered filaments
        if (config.user_ordering) {
            const auto order = config_store().filament_order.get();
            constexpr EncodedFilamentType no_filament;
            for (auto it = order.begin(); it != order.end() && *it != no_filament; it++) {
                f(it->decode());
            }
        }

        // Then walk all filaments - user ordered filaments should already be in the result, so they shall be skipped
        for (FilamentType ft : all_filament_types) {
            f(ft);
        }
    };

    if (config.enforce_first_item) {
        // Kept at position 0 regardless of compatibility, so it bypasses append_filament's filter.
        list.push_back(config.enforce_first_item);
        is_filament_in_list_bitset.set(EncodedFilamentType(config.enforce_first_item).data);
    }

    // Append visible first, if requested
    if (config.visible_first && !config.visible_only) {
        walk_filaments([&](FilamentType ft) {
            if (is_filament_visible(ft)) {
                append_filament(ft);
            }
        });
    }

    // Append the rest
    walk_filaments([&](FilamentType ft) {
        if (!config.visible_only || is_filament_visible(ft)) {
            append_filament(ft);
        }
    });

    // Unless we're filtering, we should always end up returning all the filaments
    debug_assert(list.size() == all_filament_types.size() || config.visible_only || !std::holds_alternative<NoTool>(config.compatible_with_tool));
}
#endif
