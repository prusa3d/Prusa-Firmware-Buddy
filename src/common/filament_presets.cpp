#include "filament.hpp"

#include <utility>

#include <common/aggregate_arity.hpp>
#include <hotend_detect.hpp>
#include <option/has_indx.h>
#include <option/has_loadcell.h>

#include "../../include/printers.h"

#ifndef UNITTESTS
    #include <inc/MarlinConfig.h>
#endif

namespace {

struct PresetFilamentParameters {
    decltype(FilamentTypeParameters::name) name;
    decltype(FilamentTypeParameters::nozzle_temperature) nozzle_temperature;
    decltype(FilamentTypeParameters::nozzle_preheat_temperature) nozzle_preheat_temperature = 170;
    decltype(FilamentTypeParameters::heatbed_temperature) heatbed_temperature;
#if HAS_FILAMENT_HEATBREAK_PARAM()
    decltype(FilamentTypeParameters::heatbreak_temperature) heatbreak_temperature = DEFAULT_HEATBREAK_TEMPERATURE;
#endif
#if HAS_CHAMBER_API()
    decltype(FilamentTypeParameters::chamber_min_temperature) chamber_min_temperature = std::nullopt;
    decltype(FilamentTypeParameters::chamber_max_temperature) chamber_max_temperature = std::nullopt;
    decltype(FilamentTypeParameters::chamber_target_temperature) chamber_target_temperature = std::nullopt;
#endif
#if HAS_CHAMBER_FILTRATION_API()
    decltype(FilamentTypeParameters::requires_filtration) requires_filtration = false;
#endif
    bool is_abrasive : 1 = false;
    bool is_flexible : 1 = false;
#if HAS_HT_HOTEND()
    bool requires_ht_idler_door : 1 = false;
#endif
};

// These temperatures correspond to slicer defaults for MBL.
constexpr PresetFilamentParametersT<PresetFilamentParameters> preset_filament_parameters_constexpr {
    {
        PresetFilamentType::PLA,
        PresetFilamentParameters {
            .name = "PLA",
            .nozzle_temperature = 215,
            .heatbed_temperature = 60,
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
            PresetFilamentParameters {
                .name = "PETG",
                .nozzle_temperature = 230,
                .heatbed_temperature = 85,
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
            PresetFilamentParameters {
                .name = "ASA",
                .nozzle_temperature = 260,
                .heatbed_temperature = 100,
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
            PresetFilamentParameters {
                .name = "PC",
                .nozzle_temperature = 275,
                .nozzle_preheat_temperature = HAS_LOADCELL() ? (HAS_INDX() ? 230 : 275 - 25) : 170,
                .heatbed_temperature = 100,
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
            PresetFilamentParameters {
                .name = "PVB",
                .nozzle_temperature = 215,
                .heatbed_temperature = 75,
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 15,
                .chamber_max_temperature = 38,
                .chamber_target_temperature = 20,
#endif
            },
        },
        {
            PresetFilamentType::ABS,
            PresetFilamentParameters {
                .name = "ABS",
                .nozzle_temperature = 255,
                .heatbed_temperature = 100,
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
            PresetFilamentParameters {
                .name = "HIPS",
                .nozzle_temperature = 220,
                .heatbed_temperature = 100,
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
            PresetFilamentParameters {
                .name = "PP",
                .nozzle_temperature = 240,
                .heatbed_temperature = 100,
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
            PresetFilamentParameters {
                .name = "FLEX",
                .nozzle_temperature = 240,
                .nozzle_preheat_temperature = HAS_LOADCELL() ? (HAS_INDX() ? 170 : 210) : 170,
                .heatbed_temperature = 50,
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
            PresetFilamentParameters {
                .name = "PA",
                // MINI has slightly lower max nozzle temperature but it is still OK for polyamid
                .nozzle_temperature = PRINTER_IS_PRUSA_MINI() ? 280 : 285,
                .nozzle_preheat_temperature = PRINTER_IS_PRUSA_MINI() ? 280 - 25 : 285 - 25,
                .heatbed_temperature = 100,
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
            PresetFilamentParameters {
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
            PresetFilamentParameters {
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
        {
            PresetFilamentType::PVA,
            PresetFilamentParameters {
                .name = "PVA",
                .nozzle_temperature = 210,
                .nozzle_preheat_temperature = 160,
                .heatbed_temperature = 60,
#if HAS_FILAMENT_HEATBREAK_PARAM()
                .heatbreak_temperature = 45,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 15,
                .chamber_max_temperature = 38,
                .chamber_target_temperature = 25,
#endif
            },
        },
        {
            PresetFilamentType::BVOH,
            PresetFilamentParameters {
                .name = "BVOH",
                .nozzle_temperature = 210,
                .nozzle_preheat_temperature = 160,
                .heatbed_temperature = 60,
#if HAS_FILAMENT_HEATBREAK_PARAM()
                .heatbreak_temperature = 45,
#endif
#if HAS_CHAMBER_API()
                .chamber_min_temperature = 15,
                .chamber_max_temperature = 38,
                .chamber_target_temperature = 25,
#endif
            },
        },
};

} // namespace

FilamentTypeParameters preset_filament_parameters(PresetFilamentType t) {
    static_assert(aggregate_arity<PresetFilamentParameters>() == //
            6 //
                + HAS_FILAMENT_HEATBREAK_PARAM() //
                + 3 * HAS_CHAMBER_API() //
                + HAS_CHAMBER_FILTRATION_API() //
                + HAS_HT_HOTEND(),
        "Revise the conversion");

    const PresetFilamentParameters &p = preset_filament_parameters_constexpr[t];
    return FilamentTypeParameters {
        .name = p.name,
        .nozzle_temperature = p.nozzle_temperature,
        .nozzle_preheat_temperature = p.nozzle_preheat_temperature,
        .heatbed_temperature = p.heatbed_temperature,
#if HAS_FILAMENT_BASE_PRESET_PARAM()
        .base_preset = t,
#endif
#if HAS_FILAMENT_HEATBREAK_PARAM()
        .heatbreak_temperature = p.heatbreak_temperature,
#endif
#if HAS_CHAMBER_API()
        .chamber_min_temperature = p.chamber_min_temperature,
        .chamber_max_temperature = p.chamber_max_temperature,
        .chamber_target_temperature = p.chamber_target_temperature,
#endif
#if HAS_CHAMBER_FILTRATION_API()
        .requires_filtration = p.requires_filtration,
#endif
        .is_abrasive = p.is_abrasive,
        .is_flexible = p.is_flexible,
#if HAS_HT_HOTEND()
        .requires_ht_idler_door = p.requires_ht_idler_door,
#endif
    };
}

#ifndef UNITTESTS

namespace {

template <PresetFilamentType type>
consteval void sanity_check_preset() {
    // Using a lambda causes the failure to be somewhat derivable from the compilation log
    auto check = [](bool cond) {
        if (!cond) {
            std::abort();
        }
    };

    // Check that no filament appears twice in the preset list
    const size_t in_preset_list_times = std::ranges::count(preset_filament_types, type);
    check(in_preset_list_times <= 1);
    if (in_preset_list_times == 0) {
        return;
    }

    const PresetFilamentParameters &params = preset_filament_parameters_constexpr[type];

    {
        int16_t max_nozzle = 305;
    #if HAS_HT_HOTEND()
        // Standard presets must fit the NTC hotend (HEATER_0_MAXTEMP); the HT-only PPS/PPA the PT1000.
        // A HAS_HT_HOTEND build can still boot a standard hotend, so only PPS/PPA get the higher ceiling.
        if (type == PresetFilamentType::PPS || type == PresetFilamentType::PPA) {
            max_nozzle = ht_hotend_max_nozzle_temp;
        }
    #endif

        check(params.nozzle_temperature <= max_nozzle - HEATER_MAXTEMP_SAFETY_MARGIN);
        check(params.nozzle_preheat_temperature <= max_nozzle - HEATER_MAXTEMP_SAFETY_MARGIN);
        check(params.heatbed_temperature <= BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN);
    }

    #if HAS_CHAMBER_API()
    if (params.chamber_min_temperature.has_value() || params.chamber_max_temperature.has_value() || params.chamber_target_temperature.has_value()) {
        // If one chamber parameter is specified, all should be specified
        check(params.chamber_min_temperature.has_value() && params.chamber_max_temperature.has_value() && params.chamber_target_temperature.has_value());
        check(*params.chamber_min_temperature <= *params.chamber_target_temperature);
        check(*params.chamber_target_temperature <= *params.chamber_max_temperature);
    }
    #endif
}

static_assert(
    []<size_t... i>(std::index_sequence<i...>) {
        (sanity_check_preset<static_cast<PresetFilamentType>(i)>(), ...);
        return true;
    }(std::make_index_sequence<std::to_underlying(PresetFilamentType::_count_sparse)>()));

} // namespace

#endif
