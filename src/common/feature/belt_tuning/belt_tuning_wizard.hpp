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
    static constexpr float frequency_mult = 2;

    using EncodedFrequency = uint8_t;

    /// Hz * frequency_mult
    EncodedFrequency encoded_frequency = 0;

    /// 0-255
    uint8_t progress_0_255 = 0;

    /// 0-100
    uint8_t last_amplitude_percent = 0;
};

/// Structure representing a FSM state for PhaseBeltTuning::results
struct BeltTuningWizardResultsData {
    static constexpr float frequency_mult = 128;

    /// 2B needed here for precision, we're using this value for computations
    using EncodedFrequency = uint16_t;

    /// Hz * frequency_mult
    EncodedFrequency encoded_frequency = 0;

    uint8_t belt_system = 0;
};

/// Runs the belt tuning wizard with the specified parameters
void belt_tuning_wizard(const MeasureBeltTensionParams &config);
