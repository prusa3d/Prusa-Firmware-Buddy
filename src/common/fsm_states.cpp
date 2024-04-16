#include <common/fsm_states.hpp>

#include <option/has_phase_stepping.h>

namespace fsm {

static constexpr uint32_t score(ClientFSM fsm_type) {
    // This roughly correspons to the original FSM queues.
    // TODO In the future, we could impose a total ordering based
    //      on the underlying value of the enum.
    switch (fsm_type) {
    case ClientFSM::Serial_printing:
    case ClientFSM::Printing:
    case ClientFSM::Selftest:
        return 0;

    case ClientFSM::Load_unload:
    case ClientFSM::Preheat:
    case ClientFSM::ESP:
    case ClientFSM::CrashRecovery:
    case ClientFSM::QuickPause:
    case ClientFSM::PrintPreview:
    case ClientFSM::ColdPull:
#if HAS_PHASE_STEPPING()
    case ClientFSM::PhaseStepping:
#endif
        return 1;

    case ClientFSM::Warning:
        return 2;

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

} // namespace fsm
