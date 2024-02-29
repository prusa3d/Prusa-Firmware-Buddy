#include "fsm_types.hpp"
#include "marlin_server.hpp"
#include "PrusaGcodeSuite.hpp"
#include "safety_timer_stubbed.hpp"
#include "../../module/stepper.h"

/**
 * @brief M0
 *
 * Use M0 for Quick pause during printing - it pauses queue processing when g-code is read
 * Resume with knob click in Quick Pause dialog
 * Parameters are not supported (original M0 has S<seconds> and P<miliseconds>)
 * Add parameter logic from "M0_M1.cpp" if you need it
 */
void PrusaGcodeSuite::M0() {
    fsm::PhaseData data = {};
    const char *msg_ptr;
    if (parser.string_arg) {
        msg_ptr = parser.string_arg;
    } else {
        msg_ptr = nullptr;
    }
    memcpy(&data[0], &msg_ptr, sizeof(uint32_t));

    FSM_HOLDER_WITH_DATA__LOGGING(QuickPause, PhasesQuickPause::QuickPaused, data);
    planner.synchronize();

    while (marlin_server::ClientResponseHandler::GetResponseFromPhase(PhasesQuickPause::QuickPaused) == Response::_none) {
        SafetyTimer::Instance().ReInit();
        idle(true, true);
    }
}
