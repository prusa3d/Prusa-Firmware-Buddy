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
        // dont call gui_loop, we want to ignore knob for now
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

static void DialogBlocking(ClientFSM fsm, std::optional<ClientFSM> skipIfOpen = std::nullopt) {
    ClientFSM skip = skipIfOpen ? *skipIfOpen : fsm;

    // wait until dialog opens
    while (!DialogHandler::Access().IsOpen(fsm) && !DialogHandler::Access().IsOpen(skip)) {
        gui::TickLoop();
        DialogHandler::Access().Loop(); // fsm events .. to be able to change state
        // dont call gui_loop, we want to ignore knob for now
    }

    // if we want to skip and dialog we want to skip is open, then skip
    if (skipIfOpen && DialogHandler::Access().IsOpen(skip)) {
        return;
    }

    // wait until dialog closes
    while (DialogHandler::Access().IsOpen(fsm)) {
        gui::TickLoop();
        DialogHandler::Access().Loop(); // fsm events .. to be able to change state
        gui_loop();
    }
}

static PreheatStatus::Result DialogBlockingLoadUnload(const char *mode_format, RetAndCool_t retAndCool) {
    PreheatStatus::Result ret = PreheatStatus::Result::DidNotFinish;
    PreheatStatus::ConsumeResult(); // clear result
    marlin_gcode_printf(mode_format, uint8_t(retAndCool));

    // ask for temperature, but we could skip opening the preheat dialog
    DialogBlocking(ClientFSM::Preheat, ClientFSM::Load_unload);

    ret = PreheatStatus::ConsumeResult();

    if (ret != PreheatStatus::Result::Error || ret != PreheatStatus::Result::Aborted || ret != PreheatStatus::Result::CooledDown || ret != PreheatStatus::Result::DidNotFinish) {
        DialogBlocking(ClientFSM::Load_unload);
        ret = PreheatStatus::ConsumeResult();
    }
    return ret;
}

namespace PreheatStatus {
Result DialogBlockingPreheat(RetAndCool_t retAndCool) {
    return DialogBlockingPreheat("M1700 W%d S", retAndCool);
}

Result DialogBlockingLoad(RetAndCool_t retAndCool) {
    return DialogBlockingLoadUnload("M701 W%d", retAndCool);
}

Result DialogBlockingUnLoad(RetAndCool_t retAndCool) {
    return DialogBlockingLoadUnload("M702 W%d", retAndCool);
}

Result DialogBlockingChangeLoad(RetAndCool_t retAndCool) {
    PreheatStatus::Result ret = PreheatStatus::Result::DidNotFinish;
    PreheatStatus::ConsumeResult(); // clear result
    marlin_gcode_printf("M1600 W%d", retAndCool);

    // wait for M1600 filament check to finish
    while (ret == Result::DidNotFinish) {
        osDelay(0);
        ret = PreheatStatus::ConsumeResult();
    }

    if (ret == Result::DoneNoFilament) {
        return ret;
    }

    DialogBlocking(ClientFSM::Load_unload);

    DialogBlocking(ClientFSM::Preheat, ClientFSM::Load_unload);

    ret = PreheatStatus::ConsumeResult();

    if (ret != PreheatStatus::Result::Error || ret != PreheatStatus::Result::Aborted || ret != PreheatStatus::Result::CooledDown || ret != PreheatStatus::Result::DidNotFinish) {
        DialogBlocking(ClientFSM::Load_unload);
    }
    return ret;
}

}
