#include "belt_tuning_wizard.hpp"

#include <module/stepper.h>
#include <module/prusa/accelerometer.h>
#include <Marlin/src/gcode/gcode.h>
#include <gcode/calibrate/M958.hpp>

namespace {

class FSMBeltTuning final {

public:
    using Phase = PhaseBeltTuning;
    using PhaseOpt = std::optional<Phase>;

    using Config = FSMHandlerPhasesConfig<Phase, FSMBeltTuning>;
    using Meta = FSMHandlerMetadata<Phase, FSMBeltTuning>;
    using PhaseConfig = FSMHandlerPhaseConfig<Phase, FSMBeltTuning>;

public:
    FSMBeltTuning(const MeasureBeltTensionParams &config)
        : fsm_(*this)
        , config_(config) {
    }

    void run() {
        while (fsm_.loop()) {
            idle(true);
            gcode.reset_stepper_timeout();
        }
    }

private:
    PhaseOpt phase_init(const Meta::LoopCallbackArgs &) {
        return Phase::ask_for_gantry_align;
    }

    void phase_ask_for_gantry_align_init(const Meta::InitCallbackArgs &) {
        // Disable the XY gantry - the user is prompted to move it by hand in this phase
        planner.synchronize();
        disable_XY();
    }

    PhaseOpt phase_ask_for_gantry_align(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or<Response>(Response::_none)) {

        case Response::Done:
            return Phase::preparing;

        case Response::Abort:
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_preparing(const Meta::LoopCallbackArgs &) {
        config_.skip_tuning = true;
        config_.skip_setup = false;
        config_.progress_callback = [this](auto...) {
            aborted_ |= (marlin_server::get_response_from_phase(fsm_.phase()) == Response::Abort);
            return !aborted_;
        };

        const auto setup_result = measure_belt_tension(config_);

        if (aborted_) {
            return Phase::finish;

        } else if (!setup_result) {
            return Phase::error;

        } else {
            return Phase::ask_for_dampeners_installation;
        }
    }

    PhaseOpt phase_ask_for_dampeners_installation(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or<Response>(Response::_none)) {

        case Response::Done:
            dampeners_installed_ = true;
            return Phase::calibrating_accelerometer;

        case Response::Abort:
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_calibrating_accelerometer(const Meta::LoopCallbackArgs &) {
        PrusaAccelerometer accelerometer;
        float prev_progress = -1;

        const auto calib_result = maybe_calibrate_and_get_accelerometer_sample_period(accelerometer, true, [&](float progress) {
            aborted_ |= (marlin_server::get_response_from_phase(fsm_.phase()) == Response::Abort);
            if (aborted_) {
                return false;
            }

            if (abs(progress - prev_progress) >= 0.01f) {
                prev_progress = progress;
                fsm_.change_data(fsm::serialize_data(BeltTuningWizardCalibratingData {
                    .progress_0_255 = static_cast<uint8_t>(progress * 255),
                }));
            }

            idle(true, true);
            return true;
        });

        if (isnan(calib_result)) {
            return Phase::error;
        }

        config_.calibrate_accelerometer = false;
        return Phase::measuring;
    }

    PhaseOpt phase_measuring(const Meta::LoopCallbackArgs &) {
        config_.skip_tuning = false;
        config_.skip_setup = true;

        std::optional<MeasureBeltTensionParams::ProgressCallbackArgs> prev_args;
        config_.progress_callback = [&](const MeasureBeltTensionParams::ProgressCallbackArgs &args) {
            aborted_ |= (marlin_server::get_response_from_phase(fsm_.phase()) == Response::Abort);
            if (aborted_) {
                return false;
            }

            static constexpr float max_expected_amplitude = 0.2f;

            if (args != prev_args) {
                prev_args = args;
                fsm_.change_data(fsm::serialize_data(BeltTuninigWizardMeasuringData {
                    .encoded_frequency = static_cast<BeltTuninigWizardMeasuringData::EncodedFrequency>(args.last_frequency * BeltTuninigWizardMeasuringData::frequency_mult),
                    .progress_0_255 = static_cast<uint8_t>(args.overall_progress * 255),
                    .last_amplitude_percent = static_cast<uint8_t>(std::clamp<float>(args.last_result * 100.0f / max_expected_amplitude, 0, 100)),
                }));
            }
            return true;
        };

        result_ = measure_belt_tension(config_);

        if (aborted_) {
            return Phase::ask_for_dampeners_uninstallation;

        } else if (!result_) {
            return Phase::error;

        } else {
            return Phase::results;
        }
    }

    void phase_results_init(const Meta::InitCallbackArgs &) {
        fsm_.change_data(fsm::serialize_data(BeltTuningWizardResultsData {
            .encoded_frequency = static_cast<BeltTuningWizardResultsData::EncodedFrequency>(result_->resonant_frequency_hz * BeltTuningWizardResultsData::frequency_mult),
            .belt_system = config_.belt_system,
        }));
    }

    PhaseOpt phase_results(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or<Response>(Response::_none)) {

        case Response::Finish:
            return Phase::ask_for_dampeners_uninstallation;

        case Response::Retry:
            return Phase::measuring;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_ask_for_dampeners_uninstallation(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or<Response>(Response::_none)) {

        case Response::Done:
            return Phase::finish;

        default:
            return std::nullopt;
        }
    }

    PhaseOpt phase_error(const Meta::LoopCallbackArgs &args) {
        switch (args.response.value_or<Response>(Response::_none)) {

        case Response::Abort:
            return dampeners_installed_ ? Phase::finish : Phase::ask_for_dampeners_uninstallation;

        case Response::Retry:
            if (!dampeners_installed_) {
                return Phase::preparing;

            } else if (config_.calibrate_accelerometer) {
                return Phase::calibrating_accelerometer;

            } else {
                return Phase::measuring;
            }

        default:
            return std::nullopt;
        }
    }

private:
    using C = FSMBeltTuning;
    static constexpr Config config {
        { Phase::init, { &C::phase_init } },
        { Phase::ask_for_gantry_align, { &C::phase_ask_for_gantry_align, &C::phase_ask_for_gantry_align_init } },
        { Phase::preparing, { &C::phase_preparing } },
        { Phase::ask_for_dampeners_installation, { &C::phase_ask_for_dampeners_installation } },
        { Phase::calibrating_accelerometer, { &C::phase_calibrating_accelerometer } },
        { Phase::measuring, { &C::phase_measuring } },
        { Phase::results, { &C::phase_results, &C::phase_results_init } },
        { Phase::ask_for_dampeners_uninstallation, { &C::phase_ask_for_dampeners_uninstallation } },
        { Phase::error, { &C::phase_error } },
        { Phase::finish, {} },
    };

private:
    FSMHandler<config> fsm_;
    MeasureBeltTensionParams config_;
    std::optional<MeasureBeltTensionResult> result_;
    bool dampeners_installed_ = false;
    bool aborted_ = false;
};

}; // namespace

void belt_tuning_wizard(const MeasureBeltTensionParams &config) {
    FSMBeltTuning fsm(config);
    fsm.run();
}
