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
        Attention,
    };

    SimplePrintState get_state() {
        marlin_vars_t *vars = marlin_vars();

        switch (vars->print_state) {
        case mpsPrinting:
            return SimplePrintState::Printing;
        case mpsPowerPanic_acFault:
        case mpsPowerPanic_Resume:
        case mpsPowerPanic_AwaitingResume:
        case mpsPausing_Begin:
        case mpsPausing_WaitIdle:
        case mpsPausing_ParkHead:
        case mpsPausing_Failed_Code:
        case mpsCrashRecovery_Begin:
        case mpsCrashRecovery_Axis_NOK:
        case mpsCrashRecovery_Retracting:
        case mpsCrashRecovery_Lifting:
        case mpsCrashRecovery_XY_Measure:
        case mpsCrashRecovery_XY_HOME:
        case mpsCrashRecovery_Repeated_Crash:
        case mpsResuming_Begin:
        case mpsResuming_Reheating:
        case mpsResuming_UnparkHead_XY:
        case mpsResuming_UnparkHead_ZE:
        case mpsAborting_Begin:
        case mpsAborting_WaitIdle:
        case mpsAborting_ParkHead:
        case mpsFinishing_WaitIdle:
        case mpsFinishing_ParkHead:
            return SimplePrintState::Busy;
        case mpsPaused:
            return SimplePrintState::Paused;
        case mpsAborted:
        case mpsFinished:
        case mpsIdle:
        case mpsExit:
            return SimplePrintState::Idle;
        case mpsPrintPreviewQuestions:
            return SimplePrintState::Attention;
        default:
            assert(0);
            return SimplePrintState::Idle;
        }
    }

}

// FIXME: There's a race between the check and the command. Does it matter?

bool JobCommand::stop() {
    const auto state = get_state();
    if (state == SimplePrintState::Printing || state == SimplePrintState::Paused || state == SimplePrintState::Attention) {
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
