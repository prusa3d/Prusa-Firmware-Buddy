/**
 * @file window_dlg_load_unload.cpp
 * @author Radek Vana
 * @date 2021-01-24
 */

#include "window_dlg_load_unload.hpp"
#include "marlin_client.h"
#include "gui.hpp" // gui_loop
#include "DialogHandler.hpp"

static void DialogBlocking(ClientFSM fsm, const char *mode_format, RetAndCool_t retAndCool) {
    marlin_gcode_printf(mode_format, uint8_t(retAndCool));

    // wait until dialog opens
    while (!DialogHandler::Access().IsOpen(fsm)) {
        gui::TickLoop();
        DialogHandler::Access().Loop(); // fsm events .. to be able to change state
        //dont call gui_loop, we want to ignore knob for now
    }

    // wait until dialog closes
    while (DialogHandler::Access().IsOpen(fsm)) {
        gui::TickLoop();
        DialogHandler::Access().Loop(); // fsm events .. to be able to change state
        gui_loop();
    }
}

static PreheatStatus::Result DialogBlockingPreheat(const char *mode_format, RetAndCool_t retAndCool) {
    PreheatStatus::Result ret = PreheatStatus::Result::DidNotFinish;
    PreheatStatus::ConsumeResult(); // clear result

    DialogBlocking(ClientFSM::Preheat, mode_format, retAndCool);

    ret = PreheatStatus::ConsumeResult();
    return ret;
}

namespace PreheatStatus {
Result DialogBlockingPreheat(RetAndCool_t retAndCool) {
    return DialogBlockingPreheat("M1700 W%d", retAndCool);
}

Result DialogBlockingLoad(RetAndCool_t retAndCool) {
    return DialogBlockingPreheat("M701 W%d", retAndCool);
}

Result DialogBlockingUnLoad(RetAndCool_t retAndCool) {
    return DialogBlockingPreheat("M702 W%d", retAndCool);
}

Result DialogBlockingChangeLoad(RetAndCool_t retAndCool) {
    return DialogBlockingPreheat("M1600 W%d", retAndCool);
}

}
