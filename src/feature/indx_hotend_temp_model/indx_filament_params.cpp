/// @file
#include "indx_filament_params.hpp"

#include <utils/enum_array.hpp>

#include <filament.hpp>

namespace indx {

namespace {

    constexpr FilamentParameters fallback_parameters {
        .linear_heat_capacity_J_C_m = 5.8f,
    };

    constexpr FilamentParameters no_filament_parameters {
        .linear_heat_capacity_J_C_m = 0,
    };

    constexpr PresetFilamentParametersT<FilamentParameters> preset_parameters {
        {
            PresetFilamentType::PLA,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 6.1f,
            },
        },
        {
            PresetFilamentType::PETG,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 4.3f,
            },
        },
        {
            PresetFilamentType::ASA,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 4.0f,
            },
        },
        {
            PresetFilamentType::PC,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 4.9f,
            },
        },
        {
            PresetFilamentType::PVB,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 4.5f,
            },
        },
        {
            PresetFilamentType::ABS,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 3.9f,
            },
        },
        {
            PresetFilamentType::HIPS,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 3.9f,
            },
        },
        {
            PresetFilamentType::PP,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 5.1f,
            },
        },
        {
            PresetFilamentType::FLEX,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 5.9f,
            },
        },
        {
            PresetFilamentType::PA,
            FilamentParameters {
                .linear_heat_capacity_J_C_m = 5.2f,
            },
        },
    };

} // namespace

const FilamentParameters &FilamentParameters::for_filament(const FilamentTypeParameters &filament_parameters) {
    if (auto preset = filament_parameters.base_preset) {
        return preset_parameters[*preset];
    } else {
        return fallback_parameters;
    }
}

const FilamentParameters &FilamentParameters::for_no_filament() {
    return no_filament_parameters;
}

} // namespace indx
