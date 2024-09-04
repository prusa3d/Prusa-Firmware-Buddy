#pragma once

#include "belt_tuning.hpp"

#include <client_response.hpp>
#include <fsm_handler.hpp>

/// Structure representing a FSM state for PhaseBeltTuning::calibrating_accelerometer
struct BeltTuningWizardCalibratingData {
    /// 0-255
    uint8_t progress_0_255 = 0;
};

/// Structure representing a FSM state for PhaseBeltTuning::measuring
struct BeltTuninigWizardMeasuringData {
    /// Hz
    uint8_t frequency = 0;

    /// 0-255
    uint8_t progress_0_255 = 0;

    /// 0-100
    uint8_t last_amplitude_percent = 0;
};

/// Structure representing a FSM state for PhaseBeltTuning::results
struct BeltTuningWizardResultsData {
    /// Hz
    uint8_t frequency = 0;

    // Keep one decimal place for the tension.
    // 25.5 N of tension would be way out of range, so should be safe
    static constexpr float tension_mult = 10;

    /// N * tension_mult
    uint8_t tension = 0;

    uint8_t belt_system = 0;
};

/// Runs the belt tuning wizard with the specified parameters
void belt_tuning_wizard(const MeasureBeltTensionParams &config);
