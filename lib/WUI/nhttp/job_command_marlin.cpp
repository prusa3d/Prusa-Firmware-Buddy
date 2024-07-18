#include "job_command.h"

#include "../../../src/common/marlin_client.hpp"

#include <state/printer_state.hpp>

#include <cassert>

using namespace marlin_server;

/*
 * This file contains few of the JobCommand methods, the ones that directly
 * talk to marlin. This is a hack/trick to allow separation in testing.
 */

namespace nhttp::printer {

namespace {

    enum class SimplePrintState {
        Idle,
        Printing,
        Paused,
        Busy,
        Attention,
    };

    SimplePrintState get_state() {
        SimplePrintState simple_state {};

        switch (marlin_vars().print_state) {

        case State::Printing:
            simple_state = SimplePrintState::Printing;
            break;

        case State::PowerPanic_acFault:
        case State::PowerPanic_Resume:
        case State::PowerPanic_AwaitingResume:
        case State::Pausing_Begin:
        case State::Pausing_WaitIdle:
        case State::Pausing_ParkHead:
        case State::Pausing_Failed_Code:
        case State::CrashRecovery_Begin:
        case State::CrashRecovery_Axis_NOK:
        case State::CrashRecovery_Retracting:
        case State::CrashRecovery_Lifting:
        case State::CrashRecovery_ToolchangePowerPanic:
        case State::CrashRecovery_XY_Measure:
        case State::CrashRecovery_XY_HOME:
        case State::CrashRecovery_HOMEFAIL:
        case State::CrashRecovery_Repeated_Crash:
        case State::Resuming_Begin:
        case State::Resuming_Reheating:
        case State::Resuming_UnparkHead_XY:
        case State::Resuming_UnparkHead_ZE:
        case State::Aborting_Begin:
        case State::Aborting_WaitIdle:
        case State::Aborting_ParkHead:
        case State::Aborting_Preview:
        case State::Finishing_WaitIdle:
        case State::Finishing_ParkHead:
        case State::PrintPreviewConfirmed:
            simple_state = SimplePrintState::Busy;
            break;

        case State::Paused:
            simple_state = SimplePrintState::Paused;
            break;

        case State::Aborted:
        case State::Finished:
        case State::Idle:
        case State::Exit:
            simple_state = SimplePrintState::Idle;
            break;

        case State::PrintPreviewQuestions:
            simple_state = SimplePrintState::Attention;
            break;

        default:
            assert(0);
            simple_state = SimplePrintState::Idle;
            break;
        }

        // The states here are different, but we probably still want the new attention states, so we are consistent.
        auto link_state = printer_state::get_state(false);
        if (link_state == printer_state::DeviceState::Attention) {
            simple_state = SimplePrintState::Attention;
        }

        return simple_state;
    }

} // namespace

// FIXME: There's a race between the check and the command. Does it matter?

bool JobCommand::stop() {
    const auto state = get_state();
    if (state == SimplePrintState::Printing || state == SimplePrintState::Paused || state == SimplePrintState::Attention) {
        marlin_client::print_abort();
        return true;
    } else {
        return false;
    }
}

bool JobCommand::pause() {
    if (get_state() == SimplePrintState::Printing) {
        marlin_client::print_pause();
        return true;
    } else {
        return false;
    }
}

bool JobCommand::pause_toggle() {
    switch (get_state()) {
    case SimplePrintState::Printing:
        marlin_client::print_pause();
        return true;
    case SimplePrintState::Paused:
        marlin_client::print_resume();
        return true;
    default:
        assert(0);
        // Fall through
    case SimplePrintState::Idle:
    case SimplePrintState::Busy:
        return false;
    }
}

bool JobCommand::resume() {
    if (get_state() == SimplePrintState::Paused) {
        marlin_client::print_resume();
        return true;
    } else {
        return false;
    }
}

} // namespace nhttp::printer
