#include "job_command.h"

#include "../../../src/common/marlin_client.hpp"

#include <cassert>

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
    };

    SimplePrintState get_state() {
        marlin_vars_t *vars = print_client::vars();
        print_client::loop();

        switch (vars->print_state) {
        case PrintState::Printing:
            return SimplePrintState::Printing;
        case PrintState::PowerPanic_acFault:
        case PrintState::PowerPanic_Resume:
        case PrintState::PowerPanic_AwaitingResume:
        case PrintState::Pausing_Begin:
        case PrintState::Pausing_WaitIdle:
        case PrintState::Pausing_ParkHead:
        case PrintState::Pausing_Failed_Code:
        case PrintState::CrashRecovery_Begin:
        case PrintState::CrashRecovery_Axis_NOK:
        case PrintState::CrashRecovery_Retracting:
        case PrintState::CrashRecovery_Lifting:
        case PrintState::CrashRecovery_XY_Measure:
        case PrintState::CrashRecovery_XY_HOME:
        case PrintState::CrashRecovery_Repeated_Crash:
        case PrintState::Resuming_Begin:
        case PrintState::Resuming_Reheating:
        case PrintState::Resuming_UnparkHead_XY:
        case PrintState::Resuming_UnparkHead_ZE:
        case PrintState::Aborting_Begin:
        case PrintState::Aborting_WaitIdle:
        case PrintState::Aborting_ParkHead:
        case PrintState::Finishing_WaitIdle:
        case PrintState::Finishing_ParkHead:
            return SimplePrintState::Busy;
        case PrintState::Paused:
            return SimplePrintState::Paused;
        case PrintState::Aborted:
        case PrintState::Finished:
        case PrintState::Idle:
        case PrintState::Exit:
            return SimplePrintState::Idle;
        default:
            assert(0);
            return SimplePrintState::Idle;
        }
    }

}

// FIXME: There's a race between the check and the command. Does it matter?

bool JobCommand::stop() {
    const auto state = get_state();
    if (state == SimplePrintState::Printing || state == SimplePrintState::Paused) {
        print_client::print_abort();
        return true;
    } else {
        return false;
    }
}

bool JobCommand::pause() {
    if (get_state() == SimplePrintState::Printing) {
        print_client::print_pause();
        return true;
    } else {
        return false;
    }
}

bool JobCommand::pause_toggle() {
    switch (get_state()) {
    case SimplePrintState::Printing:
        print_client::print_pause();
        return true;
    case SimplePrintState::Paused:
        print_client::print_resume();
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
        print_client::print_resume();
        return true;
    } else {
        return false;
    }
}

}
