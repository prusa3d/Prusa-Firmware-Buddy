/**
 * @file window_dlg_load_unload.cpp
 * @author Radek Vana
 * @date 2021-01-24
 */

#include "window_dlg_load_unload.hpp"
#include "marlin_client.h"
#include "gui.hpp" // gui_loop
#include "DialogHandler.hpp"

namespace PreheatStatus {

void Dialog(PreheatMode mode, RetAndCool_t retAndCool) {
    const PreheatData preheatData(mode, retAndCool);
    marlin_gcode_printf("M1400 S%d", preheatData.Data());
}

Result DialogBlocking(PreheatMode mode, RetAndCool_t retAndCool) {
    PreheatStatus::ConsumeResult(); // clear result
    Dialog(mode, retAndCool);
    PreheatStatus::Result ret;
    while ((ret = PreheatStatus::ConsumeResult()) == PreheatStatus::Result::DidNotFinish) {
        DialogHandler::Access().Loop(); // fsm events .. to be able to change state
        gui_loop();
    }
    return ret;
}

}
