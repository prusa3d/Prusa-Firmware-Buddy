#include <common/fsm_states.hpp>

#include <option/has_phase_stepping.h>
#include <option/has_input_shaper_calibration.h>
#include <logging/log.hpp>

LOG_COMPONENT_DEF(Fsm, logging::Severity::debug);

namespace fsm {

static constexpr uint32_t score(ClientFSM fsm_type) {
    // This roughly correspons to the original FSM queues.
    // TODO In the future, we could impose a total ordering based
    //      on the underlying value of the enum.
    switch (fsm_type) {
    case ClientFSM::Wait:
        return 0;

    case ClientFSM::Serial_printing:
    case ClientFSM::Printing:
    case ClientFSM::Selftest:
        return 1;

    case ClientFSM::Load_unload:
    case ClientFSM::Preheat:
#if ENABLED(CRASH_RECOVERY)
    case ClientFSM::CrashRecovery:
#endif
    case ClientFSM::QuickPause:
    case ClientFSM::PrintPreview:
#if HAS_COLDPULL()
    case ClientFSM::ColdPull:
#endif
    case ClientFSM::NetworkSetup:
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    case ClientFSM::InputShaperCalibration:
#endif
#if HAS_BELT_TUNING()
    case ClientFSM::BeltTuning:
#endif
        return 2;

    case ClientFSM::Warning:
        return 3;

    case ClientFSM::_none:
        break;
    }
    abort();
}

std::optional<States::Top> States::get_top() const {
    std::optional<Top> top;

    size_t i = 0;
    for (const State &state : states) {
        const ClientFSM fsm_type = static_cast<ClientFSM>(i++);

        if (!state) {
            continue;
        }

        if (top && score(fsm_type) <= score(top->fsm_type)) {
            continue;
        }

        top = { .fsm_type = fsm_type, .data = *state };
    }

    return top;
}

void States::log() const {
#if _DEBUG
    log_debug(Fsm, "New generation %" PRIu32, generation);
    for (size_t i = 0; i < states.size(); i++) {
        if (states[i].has_value()) {
            log_debug(Fsm, "%zu: %hhu", i, states[i]->GetPhase());
        }
    }
#endif
}

} // namespace fsm
