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
        marlin_vars_t *vars = marlin_vars();
        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE));

        switch (vars->print_state) {
        case marlin_print_state_t::Printing:
            return SimplePrintState::Printing;
        case marlin_print_state_t::PowerPanic_acFault:
        case marlin_print_state_t::PowerPanic_Resume:
        case marlin_print_state_t::PowerPanic_AwaitingResume:
        case marlin_print_state_t::Pausing_Begin:
        case marlin_print_state_t::Pausing_WaitIdle:
        case marlin_print_state_t::Pausing_ParkHead:
        case marlin_print_state_t::Pausing_Failed_Code:
        case marlin_print_state_t::CrashRecovery_Begin:
        case marlin_print_state_t::CrashRecovery_Axis_NOK:
        case marlin_print_state_t::CrashRecovery_Retracting:
        case marlin_print_state_t::CrashRecovery_Lifting:
        case marlin_print_state_t::CrashRecovery_XY_Measure:
        case marlin_print_state_t::CrashRecovery_XY_HOME:
        case marlin_print_state_t::CrashRecovery_Repeated_Crash:
        case marlin_print_state_t::Resuming_Begin:
        case marlin_print_state_t::Resuming_Reheating:
        case marlin_print_state_t::Resuming_UnparkHead_XY:
        case marlin_print_state_t::Resuming_UnparkHead_ZE:
        case marlin_print_state_t::Aborting_Begin:
        case marlin_print_state_t::Aborting_WaitIdle:
        case marlin_print_state_t::Aborting_ParkHead:
        case marlin_print_state_t::Finishing_WaitIdle:
        case marlin_print_state_t::Finishing_ParkHead:
            return SimplePrintState::Busy;
        case marlin_print_state_t::Paused:
            return SimplePrintState::Paused;
        case marlin_print_state_t::Aborted:
        case marlin_print_state_t::Finished:
        case marlin_print_state_t::Idle:
        case marlin_print_state_t::Exit:
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
        marlin_print_abort();
        return true;
    } else {
        return false;
    }
}

bool JobCommand::pause() {
    if (get_state() == SimplePrintState::Printing) {
        marlin_print_pause();
        return true;
    } else {
        return false;
    }
}

bool JobCommand::pause_toggle() {
    switch (get_state()) {
    case SimplePrintState::Printing:
        marlin_print_pause();
        return true;
    case SimplePrintState::Paused:
        marlin_print_resume();
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
        marlin_print_resume();
        return true;
    } else {
        return false;
    }
}

}
