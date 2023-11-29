#include "printer_state.hpp"

#include <fsm_types.hpp>
#include <client_response.hpp>
#include <marlin_vars.hpp>
#include <option/has_mmu2.h>
#include <config_store/store_instance.hpp>

using namespace marlin_server;

namespace printer_state {
namespace {
    StateWithAttentionCode get_print_state(State state, bool ready) {
        switch (state) {
        case State::PrintPreviewQuestions:
            return { DeviceState::Attention, AttentionCode::PrintPreviewQuestions };
        case State::PowerPanic_AwaitingResume:
            return { DeviceState::Attention, AttentionCode::PowerpanicColdBed };
        case State::CrashRecovery_Axis_NOK:
            return { DeviceState::Attention, AttentionCode::CrashRecoveryAxisNok };
        case State::CrashRecovery_Repeated_Crash:
            return { DeviceState::Attention, AttentionCode::CrashRecoveryRepeatedCrash };
        case State::CrashRecovery_HOMEFAIL:
            return { DeviceState::Attention, AttentionCode::CrashRecoveryHomeFail };
        case State::CrashRecovery_Tool_Pickup:
            return { DeviceState::Attention, AttentionCode::CrashRecoveryToolPickup };
        case State::PrintPreviewToolsMapping:
            return { DeviceState::Attention, AttentionCode::PrintPreviewToolsMapping };
        case State::Idle:
        case State::WaitGui:
        case State::PrintPreviewInit:
        case State::PrintPreviewImage:
        case State::PrintInit:
        case State::Exit:
            if (ready) {
                return { DeviceState::Ready, std::nullopt };
            } else {
                return { DeviceState::Idle, std::nullopt };
            }
        case State::Printing:
        case State::Aborting_Begin:
        case State::Aborting_WaitIdle:
        case State::Aborting_UnloadFilament:
        case State::Aborting_ParkHead:
        case State::Aborting_Preview:
        case State::Finishing_WaitIdle:
        case State::Finishing_UnloadFilament:
        case State::Finishing_ParkHead:
        case State::PrintPreviewConfirmed:
        case State::SerialPrintInit:
            return { DeviceState::Printing, std::nullopt };

        case State::PowerPanic_acFault:
        case State::PowerPanic_Resume:
        case State::CrashRecovery_Begin:
        case State::CrashRecovery_Retracting:
        case State::CrashRecovery_Lifting:
        case State::CrashRecovery_ToolchangePowerPanic:
        case State::CrashRecovery_XY_Measure:
        case State::CrashRecovery_XY_HOME:
            return { DeviceState::Busy, std::nullopt };

        case State::Pausing_Begin:
        case State::Pausing_WaitIdle:
        case State::Pausing_ParkHead:
        case State::Paused:

        case State::Resuming_Begin:
        case State::Resuming_Reheating:
        case State::Pausing_Failed_Code:
        case State::Resuming_UnparkHead_XY:
        case State::Resuming_UnparkHead_ZE:
            return { DeviceState::Paused, std::nullopt };
        case State::Finished:
            if (ready) {
                return { DeviceState::Ready, std::nullopt };
            } else {
                return { DeviceState::Finished, std::nullopt };
            }
        case State::Aborted:
            if (ready) {
                return { DeviceState::Ready, std::nullopt };
            } else {
                return { DeviceState::Stopped, std::nullopt };
            }
        }
        return { DeviceState::Unknown, std::nullopt };
    }

    // FIXME: these are also caught by the switch statement above, is there any
    // harm in having it in both places? Maybe couple more bytes of flash will
    // be used, so should we just remove it and let the get_print_state handle
    // this one, or is this better, because it's more robust?
    std::optional<AttentionCode> crash_recovery_attention(const PhasesCrashRecovery &phase) {
        switch (phase) {
        case PhasesCrashRecovery::axis_NOK:
            return AttentionCode::CrashRecoveryAxisNok;
        case PhasesCrashRecovery::repeated_crash:
            return AttentionCode::CrashRecoveryRepeatedCrash;
        case PhasesCrashRecovery::home_fail:
            return AttentionCode::CrashRecoveryHomeFail;
        case PhasesCrashRecovery::tool_recovery:
            return AttentionCode::CrashRecoveryToolPickup;
        default:
            return std::nullopt;
        }
    }

    std::optional<AttentionCode> is_attention_while_printing(const fsm::Change &q1_change) {
        assert(q1_change.get_queue_index() == fsm::QueueIndex::q1);

        switch (q1_change.get_fsm_type()) {
        case ClientFSM::Load_unload:
#if HAS_MMU2()
            if (config_store().mmu2_enabled.get()) {
                // distinguish between regular progress of MMU Load/Unload and a real attention/MMU error screen (which is only one particular FSM state)
                if (GetEnumFromPhaseIndex<PhasesLoadUnload>(q1_change.get_data().GetPhase()) == PhasesLoadUnload::MMU_ERRWaitingForUser) {
                    return AttentionCode::MMULoadUnloadError;
                } else {
                    return std::nullopt;
                }
            }
#endif
            // MMU not supported or not active -> all load/unload during print is really attention.
            return AttentionCode::FilamentRunout;
        case ClientFSM::CrashRecovery:
            return crash_recovery_attention(GetEnumFromPhaseIndex<PhasesCrashRecovery>(q1_change.get_data().GetPhase()));
        default:
            return std::nullopt;
        }
    }
} // namespace

DeviceState get_state(bool ready) {
    return get_state_with_attenion_code(ready).device_state;
}

StateWithAttentionCode get_state_with_attenion_code(bool ready) {
    auto fsm_change = marlin_vars()->get_last_fsm_change();
    State state = marlin_vars()->print_state;

    switch (fsm_change.q0_change.get_fsm_type()) {
    case ClientFSM::Printing:
        if (auto attention_code = is_attention_while_printing(fsm_change.q1_change); attention_code.has_value()) {
            return { DeviceState::Attention, attention_code.value() };
        }
        break;
    case ClientFSM::Load_unload:
    case ClientFSM::Selftest:
    case ClientFSM::ESP:
    case ClientFSM::Serial_printing:
    // FIXME: BFW-3893 Sadly there is no way (without saving state in this function)
    //  to distinguish between preheat from main screen,
    // which would be Idle, and preheat in the middle of filament load/unload,
    // so it is probably better to take it as busy, given we want to decide
    // to allow or not allow remote printing based on this, but this will cause
    // preheat menu to be the only menu screen to not be Idle... :-(
    case ClientFSM::Preheat:
        return { DeviceState::Busy, std::nullopt };
    default:
        break;
    }

    return get_print_state(state, ready);
}

bool remote_print_ready(bool preview_only) {
    auto &print_state = marlin_vars()->print_state;
    if (print_state == State::PrintPreviewInit || print_state == State::PrintPreviewImage) {
        return !preview_only;
    }

    switch (get_state(false)) {
    case DeviceState::Idle:
    case DeviceState::Ready:
    case DeviceState::Stopped:
    case DeviceState::Finished:
        return true;
    default:
        return false;
    }
}

bool has_job() {
    switch (get_state(false)) {
    case DeviceState::Printing:
    case DeviceState::Paused:
        return true;
    case DeviceState::Attention:
        // has job only if attention while printing
        return marlin_vars()->get_last_fsm_change().q0_change.get_fsm_type() == ClientFSM::Printing;
    default:
        return false;
    }
}

const char *to_str(DeviceState state) {
    switch (state) {
    case DeviceState::Idle:
        return "IDLE";
    case DeviceState::Printing:
        return "PRINTING";
    case DeviceState::Paused:
        return "PAUSED";
    case DeviceState::Finished:
        return "FINISHED";
    case DeviceState::Stopped:
        return "STOPPED";
    case DeviceState::Ready:
        return "READY";
    case DeviceState::Error:
        return "ERROR";
    case DeviceState::Busy:
        return "BUSY";
    case DeviceState::Attention:
        return "ATTENTION";
    case DeviceState::Unknown:
    default:
        return "UNKNOWN";
    }
}

const char *to_str(AttentionCode attention_code, char *buffer, size_t size) {
    snprintf(buffer, size, "A-%05d", ftrstd::to_underlying(attention_code));
    return buffer;
}

} // namespace printer_state
