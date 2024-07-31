#pragma once

#include <marlin_server.hpp>
#include <optional>
#include <enum_array.hpp>

template <typename Phase_, typename FSMClass_>
struct FSMHandlerMetadata {

    using Phase = Phase_;
    using FSMClass = FSMClass_;

    struct LoopCallbackArgs {
        /// New phase the FSM should switch to (or nullopt if it should stay in the current one)
        using Result = std::optional<Phase>;

        Phase current_phase;
        FSMResponseVariant response;
    };

    struct InitCallbackArgs {
        using Result = void;

        Phase current_phase;
        std::optional<Phase> previous_phase = std::nullopt;
    };

    struct ExitCallbackArgs {
        using Result = void;

        Phase current_phase;
        std::optional<Phase> next_phase = std::nullopt;
    };

    using LoopCalback = LoopCallbackArgs::Result (FSMClass::*)(const LoopCallbackArgs &args);
    using InitCallback = InitCallbackArgs::Result (FSMClass::*)(const InitCallbackArgs &args);
    using ExitCallback = InitCallbackArgs::Result (FSMClass::*)(const ExitCallbackArgs &args);
};

template <typename Phase, typename FSMClass>
struct FSMHandlerPhaseConfig {

public:
    using Meta = FSMHandlerMetadata<Phase, FSMClass>;

public:
    /// Function that gets called periodically while the FSM is in a given phase
    Meta::LoopCalback loop_callback;

    /// Function that gets called when the FSM is entering a given phase
    Meta::InitCallback init_callback = nullptr;

    /// Function that gets called when the FSM is leaving a given phase
    Meta::ExitCallback exit_callback = nullptr;
};

/// The \p Phase expects to have ::init && ::finish
template <typename Phase_, typename FSMClass_>
using FSMHandlerPhasesConfig = EnumArray<Phase_, FSMHandlerPhaseConfig<Phase_, FSMClass_>, CountPhases<Phase_>()>;

template <const auto &phase_config>
using FSMHandlerPhasesConfigMeta = std::remove_cvref_t<decltype(phase_config)>::value_type::Meta;

template <const auto &phase_config>
class FSMHandler {

public:
    using Meta = FSMHandlerPhasesConfigMeta<phase_config>;
    using Phase = Meta::Phase;

public:
    FSMHandler(Meta::FSMClass &fsm_class)
        : fsm_class_(fsm_class) //
    {
        marlin_server::fsm_create(phase_, {});
        if (auto f = phase_config[phase_].init_callback) {
            (fsm_class_.*f)(typename Meta::InitCallbackArgs {
                .current_phase = phase_,
            });
        }
    }
    ~FSMHandler() {
        if (auto f = phase_config[phase_].exit_callback) {
            (fsm_class_.*f)(typename Meta::ExitCallbackArgs {
                .current_phase = phase_,
            });
        }
        marlin_server::fsm_destroy(client_fsm_from_phase(Phase {}));
    }

public:
    bool loop() {
        const typename Meta::LoopCallbackArgs loop_callback_args {
            .current_phase = phase_,
            .response = marlin_server::get_response_variant_from_phase(phase_),
        };

        const auto f = phase_config[phase_].loop_callback;
        if (!f) {
            return false;
        }

        const Phase old_phase = phase_;
        const Phase new_phase = (fsm_class_.*f)(loop_callback_args).value_or(old_phase);

        if (new_phase != old_phase) {
            if (auto f = phase_config[old_phase].exit_callback) {
                (fsm_class_.*f)(typename Meta::ExitCallbackArgs {
                    .current_phase = phase_,
                    .next_phase = new_phase,
                });
            }

            phase_ = new_phase;
            marlin_server::fsm_change(new_phase);

            if (auto f = phase_config[new_phase].init_callback) {
                (fsm_class_.*f)(typename Meta::InitCallbackArgs {
                    .current_phase = phase_,
                    .previous_phase = old_phase,
                });
            }
        }

        return phase_ != Phase::finish;
    }

    /// \returns current phase of the FSM
    inline auto phase() const {
        return phase_;
    }

    /// Notifies about data change for the current phase
    void change_data(fsm::PhaseData data) {
        marlin_server::fsm_change(phase_, data);
    }

private:
    Meta::FSMClass &fsm_class_;
    Phase phase_ = Phase::init;
};
