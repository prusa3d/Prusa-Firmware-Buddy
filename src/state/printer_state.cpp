#include "printer_state.hpp"

#include <fsm_types.hpp>
#include <client_response.hpp>

using namespace marlin_server;

namespace printer_state {
namespace {
    DeviceState get_print_state(State state, bool ready) {
        switch (state) {
        case State::PrintPreviewQuestions:
            return DeviceState::Attention;
        case State::Idle:
        case State::WaitGui:
        case State::PrintPreviewInit:
        case State::PrintPreviewImage:
        case State::PrintInit:
        case State::Exit:
            if (ready) {
                return DeviceState::Ready;
            } else {
                return DeviceState::Idle;
            }
        case State::Printing:
        case State::Aborting_Begin:
        case State::Aborting_WaitIdle:
        case State::Aborting_ParkHead:
        case State::Finishing_WaitIdle:
        case State::Finishing_ParkHead:
            return DeviceState::Printing;

        case State::PowerPanic_acFault:
        case State::PowerPanic_Resume:
        case State::PowerPanic_AwaitingResume:
        case State::CrashRecovery_Begin:
        case State::CrashRecovery_Axis_NOK:
        case State::CrashRecovery_Retracting:
        case State::CrashRecovery_Lifting:
        case State::CrashRecovery_XY_Measure:
        case State::CrashRecovery_Tool_Pickup:
        case State::CrashRecovery_XY_HOME:
        case State::CrashRecovery_HOMEFAIL:
        case State::CrashRecovery_Repeated_Crash:
            return DeviceState::Busy;

        case State::Pausing_Begin:
        case State::Pausing_WaitIdle:
        case State::Pausing_ParkHead:
        case State::Paused:

        case State::Resuming_Begin:
        case State::Resuming_Reheating:
        case State::Pausing_Failed_Code:
        case State::Resuming_UnparkHead_XY:
        case State::Resuming_UnparkHead_ZE:
            return DeviceState::Paused;
        case State::Finished:
            return DeviceState::Finished;
        case State::Aborted:
            return DeviceState::Stopped;
        }
        return DeviceState::Unknown;
    }
}

DeviceState get_state(State state, const marlin_vars_t::FSMChange &fsm_change, bool ready) {
    auto print_state = get_print_state(state, ready);

    if (print_state == DeviceState::Printing && fsm_change.q1_change.get_fsm_type() == ClientFSM::Load_unload) {
        return DeviceState::Attention;
    }

    return print_state;
}

}
