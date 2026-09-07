#include "filament.hpp"

#include <hotend_detect.hpp>
#include <option/has_indx.h>
#include <option/has_loadcell.h>

#include "../../include/printers.h"

#ifndef UNITTESTS
    #include <inc/MarlinConfig.h>
#endif

// These temperatures correspond to slicer defaults for MBL.
constexpr const EnumArray<PresetFilamentType, FilamentTypeParameters, PresetFilamentType::_count> preset_filament_parameters_constexpr {
    {
        PresetFilamentType::PLA,
        FilamentTypeParameters {
            .name = "PLA",
            .nozzle_temperature = 215,
            .heatbed_temperature = 60,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
            .base_preset = PresetFilamentType::PLA,
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
            .heatbreak_temperature = 45,
#endif
#if HAS_CHAMBER_API()
            .chamber_min_temperature = 15,
            .chamber_max_temperature = 38,
            .chamber_target_temperature = 20,
#endif
        },
    },
        {
            PresetFilamentType::PETG,
            FilamentTypeParameters {
                .name = "PETG",
                .nozzle_temperature = 230,
                .heatbed_temperature = 85,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::PETG,
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                .heatbreak_temperature = 60,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 15,
                .chamber_max_temperature = 45,
                .chamber_target_temperature = 30,
#endif
            },
        },
        {
            PresetFilamentType::ASA,
            FilamentTypeParameters {
                .name = "ASA",
                .nozzle_temperature = 260,
                .heatbed_temperature = 100,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::ASA,
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                .heatbreak_temperature = 65,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 40,
                .chamber_max_temperature = 75,
                .chamber_target_temperature = 70,
#endif
#if HAS_CHAMBER_FILTRATION_API()
                .requires_filtration = true,
#endif
            },
        },
        {
            PresetFilamentType::PC,
            FilamentTypeParameters {
                .name = "PC",
                .nozzle_temperature = 275,
                .nozzle_preheat_temperature = HAS_LOADCELL() ? (HAS_INDX() ? 230 : 275 - 25) : 170,
                .heatbed_temperature = 100,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::PC,
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                .heatbreak_temperature = 65,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 40,
                .chamber_max_temperature = 80,
                .chamber_target_temperature = 75,
#endif
#if HAS_CHAMBER_FILTRATION_API()
                .requires_filtration = true,
#endif
            },
        },
        {
            PresetFilamentType::PVB,
            FilamentTypeParameters {
                .name = "PVB",
                .nozzle_temperature = 215,
                .heatbed_temperature = 75,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::PVB,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 15,
                .chamber_max_temperature = 38,
                .chamber_target_temperature = 20,
#endif
            },
        },
        {
            PresetFilamentType::ABS,
            FilamentTypeParameters {
                .name = "ABS",
                .nozzle_temperature = 255,
                .heatbed_temperature = 100,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::ABS,
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
                .heatbreak_temperature = 65,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 40,
                .chamber_max_temperature = 75,
                .chamber_target_temperature = 70,
#endif
#if HAS_CHAMBER_FILTRATION_API()
                .requires_filtration = true,
#endif
            },
        },
        {
            PresetFilamentType::HIPS,
            FilamentTypeParameters {
                .name = "HIPS",
                .nozzle_temperature = 220,
                .heatbed_temperature = 100,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::HIPS,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 40,
                .chamber_max_temperature = 75,
                .chamber_target_temperature = 70,
#endif
#if HAS_CHAMBER_FILTRATION_API()
                .requires_filtration = true,
#endif
            },
        },
        {
            PresetFilamentType::PP,
            FilamentTypeParameters {
                .name = "PP",
                .nozzle_temperature = 240,
                .heatbed_temperature = 100,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::PP,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 30,
                .chamber_max_temperature = 70,
                .chamber_target_temperature = 60,
#endif
#if HAS_CHAMBER_FILTRATION_API()
                .requires_filtration = true,
#endif
            },
        },
        {
            PresetFilamentType::FLEX,
            FilamentTypeParameters {
                .name = "FLEX",
                .nozzle_temperature = 240,
                .nozzle_preheat_temperature = HAS_LOADCELL() ? (HAS_INDX() ? 170 : 210) : 170,
                .heatbed_temperature = 50,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::FLEX,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 15,
                .chamber_max_temperature = 40,
                .chamber_target_temperature = 25,
#endif
#if HAS_CHAMBER_FILTRATION_API()
                .requires_filtration = true,
#endif
                .is_flexible = true,
            },
        },
        {
            PresetFilamentType::PA,
            FilamentTypeParameters {
                .name = "PA",
                // MINI has slightly lower max nozzle temperature but it is still OK for polyamid
                .nozzle_temperature = PRINTER_IS_PRUSA_MINI() ? 280 : 285,
                .nozzle_preheat_temperature = PRINTER_IS_PRUSA_MINI() ? 280 - 25 : 285 - 25,
                .heatbed_temperature = 100,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
                .base_preset = PresetFilamentType::PA,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 40,
                .chamber_max_temperature = 70,
                .chamber_target_temperature = 65,
#endif
            },
        },
#if HAS_HT_HOTEND()
        {
            PresetFilamentType::PPS,
            FilamentTypeParameters {
                .name = "PPS",
                .nozzle_temperature = 320,
                .heatbed_temperature = 105,
    #if HAS_CHAMBER_API()
                .chamber_min_temperature = PRINTER_IS_PRUSA_COREONEL() ? 55 : 45,
                .chamber_max_temperature = PRINTER_IS_PRUSA_COREONEL() ? 70 : 65,
                .chamber_target_temperature = PRINTER_IS_PRUSA_COREONEL() ? 60 : 55,
    #endif
                .is_abrasive = true,
                .requires_ht_idler_door = true,
            },
        },
        {
            PresetFilamentType::PPA,
            FilamentTypeParameters {
                .name = "PPA",
                .nozzle_temperature = 320,
                .heatbed_temperature = 105,
    #if HAS_CHAMBER_API()
                .chamber_min_temperature = PRINTER_IS_PRUSA_COREONEL() ? 55 : 45,
                .chamber_max_temperature = PRINTER_IS_PRUSA_COREONEL() ? 70 : 65,
                .chamber_target_temperature = PRINTER_IS_PRUSA_COREONEL() ? 60 : 55,
    #endif
                .is_abrasive = true,
                .requires_ht_idler_door = true,
            },
        },
#endif
};

constinit const EnumArray<PresetFilamentType, FilamentTypeParameters, PresetFilamentType::_count> preset_filament_parameters = preset_filament_parameters_constexpr;

#ifndef UNITTESTS

constexpr bool temperatures_are_within_spec(const FilamentTypeParameters &filament, int16_t max_nozzle) {
    return (filament.nozzle_temperature <= max_nozzle - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.nozzle_preheat_temperature <= max_nozzle - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.heatbed_temperature <= BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN);
}

// Standard presets must fit the NTC hotend (HEATER_0_MAXTEMP); the HT-only PPS/PPA the PT1000.
// A HAS_HT_HOTEND build can still boot a standard hotend, so only PPS/PPA get the higher ceiling.
constexpr bool preset_temperatures_are_within_spec() {
    for (size_t i = 0; i < static_cast<size_t>(PresetFilamentType::_count); i++) {
        int16_t max_nozzle = HEATER_0_MAXTEMP;
    #if HAS_HT_HOTEND()
        const auto type = static_cast<PresetFilamentType>(i);
        if (type == PresetFilamentType::PPS || type == PresetFilamentType::PPA) {
            max_nozzle = ht_hotend_max_nozzle_temp;
        }
    #endif
        if (!temperatures_are_within_spec(preset_filament_parameters_constexpr[i], max_nozzle)) {
            return false;
        }
    }
    return true;
}
static_assert(preset_temperatures_are_within_spec());

    #if HAS_CHAMBER_API()
constexpr bool chamber_temperatures_are_within_spec(const FilamentTypeParameters &filament) {
    // If one chamber parameter is specified, all should be specified
    if (!filament.chamber_min_temperature.has_value() && !filament.chamber_max_temperature.has_value() && !filament.chamber_target_temperature.has_value()) {
        return true;
    }
    if (!filament.chamber_min_temperature.has_value() || !filament.chamber_max_temperature.has_value() || !filament.chamber_target_temperature.has_value()) {
        return false;
    }

    return (*filament.chamber_min_temperature <= *filament.chamber_target_temperature) && (*filament.chamber_target_temperature <= *filament.chamber_max_temperature);
}
static_assert(std::ranges::all_of(preset_filament_parameters_constexpr, chamber_temperatures_are_within_spec));
    #endif

    #if HAS_FILAMENT_BASE_PRESET_PARAM()
constexpr bool presets_base_preset_matches() {
    for (size_t i = 0; i < static_cast<size_t>(PresetFilamentType::_count); i++) {
        if (preset_filament_parameters_constexpr[i].base_preset != static_cast<PresetFilamentType>(i)) {
            return false;
        }
    }
    return true;
}

static_assert(presets_base_preset_matches());
    #endif
#endif
