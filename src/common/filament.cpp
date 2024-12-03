#include "filament.hpp"
#include "filament_list.hpp"

#include <cassert>
#include <cstring>
#include <algorithm>

#include "i18n.h"
#include "../../include/printers.h"
#include <enum_array.hpp>
#include <option/has_loadcell.h>
#include <str_utils.hpp>
#include <guiconfig/guiconfig.h>
#include <config_store/store_instance.hpp>
#include <Configuration.h>

// !!! If these value change, you need to inspect usages and possibly write up some config store migrations
static_assert(filament_name_buffer_size == 8);
static_assert(max_preset_filament_type_count == 32);
static_assert(max_user_filament_type_count == 32);
static_assert(max_total_filament_count == 64);
static_assert(sizeof(FilamentTypeParameters) == 14);

static_assert(preset_filament_type_count <= max_preset_filament_type_count);
static_assert(user_filament_type_count <= max_user_filament_type_count);
static_assert(EXTRUDERS <= adhoc_filament_type_count);

// We're storing the bed temperature in uint8_t, so make sure the bed cannot go higher
static_assert(BED_MAXTEMP <= 255);

static constexpr FilamentTypeParameters none_filament_parameters {
    .name = FilamentTypeParameters::name_from_str("---"),
    .nozzle_temperature = 0,
    .nozzle_preheat_temperature = 0,
    .heatbed_temperature = 0,
};

// These temperatures correspond to slicer defaults for MBL.
constexpr EnumArray<PresetFilamentType, FilamentTypeParameters, PresetFilamentType::_count> preset_filament_parameters {
    {
        PresetFilamentType::PLA,
        {
            .name = FilamentTypeParameters::name_from_str("PLA"),
            .nozzle_temperature = 215,
            .heatbed_temperature = 60,
        },
    },
    {
        PresetFilamentType::PETG,
        {
            .name = FilamentTypeParameters::name_from_str("PETG"),
            .nozzle_temperature = 230,
            .heatbed_temperature = 85,
        },
    },
    {
        PresetFilamentType::ASA,
        {
            .name = FilamentTypeParameters::name_from_str("ASA"),
            .nozzle_temperature = 260,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PC,
        {
            .name = FilamentTypeParameters::name_from_str("PC"),
            .nozzle_temperature = 275,
            .nozzle_preheat_temperature = HAS_LOADCELL() ? 170 : 275 - 25,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PVB,
        {
            .name = FilamentTypeParameters::name_from_str("PVB"),
            .nozzle_temperature = 215,
            .heatbed_temperature = 75,
        },
    },
    {
        PresetFilamentType::ABS,
        {
            .name = FilamentTypeParameters::name_from_str("ABS"),
            .nozzle_temperature = 255,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::HIPS,
        {
            .name = FilamentTypeParameters::name_from_str("HIPS"),
            .nozzle_temperature = 220,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PP,
        {
            .name = FilamentTypeParameters::name_from_str("PP"),
            .nozzle_temperature = 240,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::FLEX,
        {
            .name = FilamentTypeParameters::name_from_str("FLEX"),
            .nozzle_temperature = 240,
            .nozzle_preheat_temperature = HAS_LOADCELL() ? 170 : 210,
            .heatbed_temperature = 50,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PA,
        {
            .name = FilamentTypeParameters::name_from_str("PA"),
            // MINI has slightly lower max nozzle temperature but it is still OK for polyamid
            .nozzle_temperature = PRINTER_IS_PRUSA_MINI() ? 280 : 285,
            .heatbed_temperature = 100,
        },
    },
};
constexpr bool temperatures_are_within_spec(const FilamentTypeParameters &filament) {
    return (filament.nozzle_temperature <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.nozzle_preheat_temperature <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.heatbed_temperature <= BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN);
}
static_assert(std::ranges::all_of(preset_filament_parameters, temperatures_are_within_spec));

FilamentTypeParameters pending_adhoc_filament_parameters;

FilamentType FilamentType::from_name(const std::string_view &name) {
    if (name.length() >= filament_name_buffer_size) {
        return FilamentType::none;
    }

    for (const FilamentType filament_type : all_filament_types) {
        if (name == filament_type.parameters().name.data()) {
            return filament_type;
        }
    }

    return FilamentType::none;
}

std::optional<FilamentType> FilamentType::from_gcode_param(const std::string_view &value) {
    if (const FilamentType r = from_name(value); r != FilamentType::none) {
        return r;
    }

    if (value == adhoc_pending_gcode_code) {
        return PendingAdHocFilamentType {};
    }

    return std::nullopt;
}

bool FilamentType::matches(const std::string_view &name) const {
    return parameters().name.data() == name;
}

void FilamentType::build_name_with_info(StringBuilder &builder) const {
    builder.append_string(parameters().name.data());

    const char *suffix =
#if HAS_MINI_DISPLAY()
        // The suffix doesn't fit on the mini display
        nullptr;
#else
        std::visit([&]<typename T>(const T &) -> const char * {
            if constexpr (std::is_same_v<T, NoFilamentType>) {
                return nullptr;

            } else if constexpr (std::is_same_v<T, PresetFilamentType>) {
                return N_(" (Preset)");

            } else if constexpr (std::is_same_v<T, UserFilamentType>) {
                return N_(" (User)");

            } else if constexpr (std::is_same_v<T, AdHocFilamentType> || std::is_same_v<T, PendingAdHocFilamentType>) {
                return N_(" (Custom)");

            } else {
                static_assert(false);
            }
        },
            *this);
#endif

    if (suffix) {
        builder.append_string_view(_(suffix));
    }
}

FilamentTypeParameters FilamentType::parameters() const {
    return std::visit([]<typename T>(const T &v) -> FilamentTypeParameters {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return preset_filament_parameters[v];

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            return config_store().user_filament_parameters.get(v.index);

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            return config_store().adhoc_filament_parameters.get(v.tool);

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            return pending_adhoc_filament_parameters;

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            return none_filament_parameters;
        }
    },
        *this);
}

void FilamentType::set_parameters(const FilamentTypeParameters &set) const {
    assert(can_be_renamed_to(set.name.data()));

    std::visit([&]<typename T>(const T &v) {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            assert(false);

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            config_store().user_filament_parameters.set(v.index, set);

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            config_store().adhoc_filament_parameters.set(v.tool, set);

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            pending_adhoc_filament_parameters = set;

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            assert(false);

        } else {
            static_assert(false);
        }
    },
        *this);
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
            return (isalnum(ch) && toupper(ch) == ch) || strchr("_-", ch);
        })) {
        return std::unexpected(N_("Name must contain only 'A-Z0-9_-' characters"));
    }

    // Check for name collisions
    if (
        // Ad-hoc filaments can "override" standard ones, so we allow name collisions for them
        !std::holds_alternative<AdHocFilamentType>(*this)

        && std::ranges::any_of(all_filament_types, [&](FilamentType ft) {
               return (ft != *this) && (new_name == ft.parameters().name.data());
           })

    ) {
        return std::unexpected(N_("Filament with this name already exists"));
    }

    return {};
}

bool FilamentType::is_visible() const {
    return std::visit([]<typename T>(const T &v) -> bool {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return config_store().visible_preset_filament_types.get().test(static_cast<size_t>(v));

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            return config_store().visible_user_filament_types.get().test(v.index);

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            return false;

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            return false;

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            return false;
        }
    },
        *this);
}

void FilamentType::set_visible(bool set) const {
    return std::visit([set]<typename T>(const T &v) -> void {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            config_store().visible_preset_filament_types.apply([v, set](auto &value) {
                value.set(static_cast<size_t>(v), set);
            });

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            config_store().visible_user_filament_types.apply([v, set](auto &value) {
                value.set(v.index, set);
            });

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            // Should never happen
            assert(0);

        } else if constexpr (std::is_same_v<T, PendingAdHocFilamentType>) {
            // Should never happen
            assert(0);

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            // Do nothing

        } else {
            static_assert(0);
        }
    },
        *this);
}
