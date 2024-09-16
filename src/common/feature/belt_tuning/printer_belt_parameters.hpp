#pragma once

#include <array>

#include <Configuration.h>

#include "belt_tuning.hpp"

struct PrinterBeltParameters {
    // Although CoreXY machines have two belts, they actually share tension, so they behave as a single belt system
    // Bedslingers have two separate belts
    static constexpr size_t belt_system_count = ENABLED(COREXY) ? 1 : 2;

    struct BeltSystemParameters {
        /// Position of the toolhead at which the measurements should be performed
        xyz_pos_t measurement_pos;

        /// (meters) Nominal length of the belt system
        float nominal_length_m;

        /// (kg/meter) Nominal weigt the a belt
        float nominal_weight_kg_m;

        /// (Newtons) Target tension force
        float target_tension_force_n;

        /// (Netwons) Deviation from the target force that is still considered acceptable.
        /// If the measured tension is within (target +- dev), then the tensioning is considered ok
        float target_tension_force_dev_n;

        /// Default parameters used for belt tuning
        MeasureBeltTensionSpecificParams belt_tuning_params;
    };

    std::array<BeltSystemParameters, belt_system_count> belt_system;
};

#if PRINTER_IS_PRUSA_XL()
static constexpr PrinterBeltParameters printer_belt_parameters {
    .belt_system = {
        PrinterBeltParameters::BeltSystemParameters {
            .measurement_pos = { .x = 342, .y = 110, .z = 10 },
            .nominal_length_m = 0.395f,
            .nominal_weight_kg_m = 0.007569f,
            .target_tension_force_n = 18,
            .target_tension_force_dev_n = 1,
            .belt_tuning_params = {
                .excitation_amplitude_m = 0.00006f,
                .start_frequency_hz = 50,
                .end_frequency_hz = 95,
                .frequency_step_hz = 0.5,
                .excitation_cycles = 50,
                .wait_cycles = 10,
                .measurement_cycles = 30,
            },
        },
    },
};

#else
    #error Mising belt tensioning parameters for the printer
#endif
