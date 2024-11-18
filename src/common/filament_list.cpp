#include "filament_list.hpp"

const GenerateFilamentListConfig management_generate_filament_list_config {
    .visible_only = false,
    .visible_first = false,
    .user_ordering = true,
};

size_t generate_filament_list(FilamentListStorage &storage, const GenerateFilamentListConfig &config) {
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
    }

    const auto is_filament_visible = [&](FilamentType ft) {
        return is_filament_visible_bitset.test(EncodedFilamentType(ft).data);
    };

    size_t cnt = 0;
    std::bitset<256> is_filament_in_list_bitset;

    /// Appends filament to the list, if it is not already there
    const auto append_filament = [&](FilamentType ft) {
        const uint8_t ix = EncodedFilamentType(ft).data;
        if (is_filament_in_list_bitset.test(ix)) {
            return;
        }

        storage[cnt++] = ft;
        is_filament_in_list_bitset.set(ix);
    };

    /// Walks filaments, one possibly multiple times
    const auto walk_filaments = [&](auto &&f) {
        // First walk user ordered filaments
        if (config.user_ordering) {
            const auto order = config_store().filament_order.get();
            for (auto it = order.begin(); *it; it++) {
                f(*it);
            }
        }

        // Then walk all filaments - user ordered filaments should already be in the result, so they shall be skipped
        for (FilamentType ft : all_filament_types) {
            f(ft);
        }
    };

    if (config.enforce_first_item) {
        append_filament(config.enforce_first_item);
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

    // Unless we're listing only visible filaments, we should always end up returning all the filaments
    assert(cnt == all_filament_types.size() || config.visible_only);

    return cnt;
}
