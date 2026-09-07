/// @file Implements functions from filament.hpp that are not buddy-dependent

#include "filament.hpp"
#include "filament_list.hpp"

#include <i18n.h>

FilamentType FilamentType::from_name(const std::string_view &name) {
    if (name.length() >= filament_name_buffer_size) {
        return FilamentType::none;
    }

    for (const FilamentType filament_type : all_filament_types) {
        if (name == filament_type.parameters().name) {
            return filament_type;
        }
    }

    return FilamentType::none;
}

std::expected<void, const char *> FilamentType::can_be_renamed_to(const std::string_view &new_name) const {
    if (!is_customizable()) {
        return std::unexpected(N_("Filament is not customizable"));
    }

    // Name must not be empty
    if (new_name.size() == 0) {
        return std::unexpected(N_("Name must not be empty"));
    }

    // Name must not be "---"
    if (new_name == "---") {
        return std::unexpected(N_("Name must not be '---'"));
    }

    // Check for valid symbols
    if (!std::ranges::all_of(new_name, [](char ch) {
            return (isalnum(ch)) || strchr("_-", ch);
        })) {
        return std::unexpected(N_("Name must contain only 'a-zA-Z0-9_-' characters"));
    }

    // Check for name collisions
    if (
        // Ad-hoc filaments can "override" standard ones, so we allow name collisions for them
        !std::holds_alternative<AdHocFilamentType>(*this) && !std::holds_alternative<PendingAdHocFilamentType>(*this)

        && std::ranges::any_of(all_filament_types, [&](FilamentType ft) {
               return (ft != *this) && (new_name == ft.parameters().name);
           })

    ) {
        return std::unexpected(N_("Filament with this name already exists"));
    }

    return {};
}
