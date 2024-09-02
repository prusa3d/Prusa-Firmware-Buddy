#include "filament.hpp"
#include "filament_list.hpp"

#include <cassert>
#include <cstring>
#include <utility>

#include "i18n.h"
#include "client_response_texts.hpp"
#include "../../include/printers.h"
#include <Marlin/src/inc/MarlinConfigPre.h>
#include <enum_array.hpp>
#include <option/has_loadcell.h>

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
    .name = "---",
    .nozzle_temperature = 0,
    .nozzle_preheat_temperature = 0,
    .heatbed_temperature = 0,
};

// These temperatures correspond to slicer defaults for MBL.
constexpr EnumArray<PresetFilamentType, FilamentTypeParameters, PresetFilamentType::_count> preset_filament_parameters {
    {
        PresetFilamentType::PLA,
        {
            .name = "PLA",
            .nozzle_temperature = 215,
            .heatbed_temperature = 60,
        },
    },
    {
        PresetFilamentType::PETG,
        {
            .name = "PETG",
            .nozzle_temperature = 230,
            .heatbed_temperature = 85,
        },
    },
    {
        PresetFilamentType::ASA,
        {
            .name = "ASA",
            .nozzle_temperature = 260,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PC,
        {
            .name = "PC",
            .nozzle_temperature = 275,
            .nozzle_preheat_temperature = HAS_LOADCELL() ? 170 : 275 - 25,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PVB,
        {
            .name = "PVB",
            .nozzle_temperature = 215,
            .heatbed_temperature = 75,
        },
    },
    {
        PresetFilamentType::ABS,
        {
            .name = "ABS",
            .nozzle_temperature = 255,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::HIPS,
        {
            .name = "HIPS",
            .nozzle_temperature = 220,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PP,
        {
            .name = "PP",
            .nozzle_temperature = 240,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::FLEX,
        {
            .name = "FLEX",
            .nozzle_temperature = 240,
            .nozzle_preheat_temperature = HAS_LOADCELL() ? 170 : 210,
            .heatbed_temperature = 50,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PA,
        {
            .name = "PA",
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

bool FilamentType::matches(const std::string_view &name) const {
    return parameters().name == name;
}

void FilamentType::build_name_with_info(StringBuilder &builder) const {
    builder.append_string(parameters().name);

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

            } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
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

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            return none_filament_parameters;
        }
    },
        *this);
}

void FilamentType::set_parameters(const FilamentTypeParameters &set) const {
    std::visit([&]<typename T>(const T &v) {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            assert(false);

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            config_store().user_filament_parameters.set(v.index, set);

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
            config_store().adhoc_filament_parameters.set(v.tool, set);

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            assert(false);

        } else {
            static_assert(false);
        }
    },
        *this);
}

bool FilamentType::is_visible() const {
    return std::visit([]<typename T>(const T &v) -> bool {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return config_store().visible_preset_filament_types.get().test(static_cast<size_t>(v));

        } else if constexpr (std::is_same_v<T, UserFilamentType>) {
            return config_store().visible_user_filament_types.get().test(v.index);

        } else if constexpr (std::is_same_v<T, AdHocFilamentType>) {
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

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            // Do nothing

        } else {
            static_assert(0);
        }
    },
        *this);
}
