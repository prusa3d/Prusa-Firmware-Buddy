/**
 * @file window_dlg_load_unload.cpp
 * @author Radek Vana
 * @date 2021-01-24
 */

#include "window_dlg_load_unload.hpp"
#include "marlin_client.h"
#include "gui.hpp" // gui_loop
#include "DialogHandler.hpp"

static PreheatStatus::Result DialogBlocking(const char *mode_format, RetAndCool_t retAndCool) {
    PreheatStatus::Result ret = PreheatStatus::Result::DidNotFinish;
    PreheatStatus::ConsumeResult(); // clear result
    marlin_gcode_printf(mode_format, uint8_t(retAndCool));
    while ((ret = PreheatStatus::ConsumeResult()) == PreheatStatus::Result::DidNotFinish) {
        gui::TickLoop();
        DialogHandler::Access().Loop(); // fsm events .. to be able to change state
        gui_loop();
    }
    return ret;
}

namespace PreheatStatus {
Result DialogBlockingPreheat(RetAndCool_t retAndCool) {
    return DialogBlocking("M1700 W%d", retAndCool);
}

Result DialogBlockingLoad(RetAndCool_t retAndCool) {
    return DialogBlocking("M701 W%d", retAndCool);
}

Result DialogBlockingUnLoad(RetAndCool_t retAndCool) {
    return DialogBlocking("M702 W%d", retAndCool);
}

Result DialogBlockingChangeLoad(RetAndCool_t retAndCool) {
    return DialogBlocking("M1600 W%d", retAndCool);
}

}
