/**
 * @file screen_test_selftest.cpp
 */

#include "screen_test_selftest.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "DialogHandler.hpp"
#include "ScreenShot.hpp"
#include "selftest_state_names.hpp"
#include "selftest_result_type.hpp"
#include "selftest_loadcell_type.hpp"

#include <dirent.h>

static void WaitLoop(uint32_t switch_period = 1024) {
    uint32_t switch_time = gui::GetTick_IgnoreTickLoop();
    while ((gui::GetTick_IgnoreTickLoop() - switch_time) < switch_period) {
        gui::TickLoop();
        gui_loop();
        DialogHandler::Access().Loop();
    }
}

static void WaitAndShot(const char *file_name, uint32_t wait = 512) {
    char buff[64];
    snprintf(buff, sizeof(buff), "/usb/selftest/%s.bmp", file_name);
    WaitLoop(wait);
    TakeAScreenshotAs(buff);
}

static void SelftestTest() {
    mkdir("/usb/selftest", 777);
    fsm::Change change(fsm::QueueIndex::q0);
    fsm::BaseData data;
    // push create
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Selftest, data);
    DialogHandler::Command(change.serialize());

    WaitLoop();
    Screens::Access()->Loop(); // for test only, this is very unsafe, do not use it like this anywhere else !!!

    for (PhasesSelftest i = PhasesSelftest::_first; int(i) <= int(PhasesSelftest::_last); i = PhasesSelftest(int(i) + 1)) {
        data.SetPhase(int(i) - int(PhasesSelftest::_first));

        // push change
        change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Selftest, data);
        DialogHandler::Command(change.serialize());

        WaitAndShot(get_selftest_state_name(i));
    }

    // alternative state for Loadcell_user_tap_ask_abort
    SelftestLoadcell_t loadcell_data;
    loadcell_data.pressed_too_soon = true;
    data.SetData(loadcell_data.Serialize());
    data.SetPhase(int(PhasesSelftest::Loadcell_user_tap_ask_abort) - int(PhasesSelftest::_first));
    // push change
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Selftest, data);
    DialogHandler::Command(change.serialize());
    WaitAndShot("Loadcell_user_tap_ask_abort__soon");

    // alternative stated for Result (default state is unknown == some tests did not pass)
    data.SetPhase(int(PhasesSelftest::Result) - int(PhasesSelftest::_first));
    data.SetData(FsmSelftestResult(0xaa).Serialize()); // 0xaa is 4 times Passed state
    // push change
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Selftest, data);
    DialogHandler::Command(change.serialize());
    WaitAndShot("Result_passed");

    // result does not have change method
    // we need to change state first
    data.SetPhase(0);
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Selftest, data);
    DialogHandler::Command(change.serialize());
    WaitLoop();

    // 1x failed
    data.SetPhase(int(PhasesSelftest::Result) - int(PhasesSelftest::_first));
    data.SetData(FsmSelftestResult(0xa8).Serialize()); // 3 passed, one failed
    // push change
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Selftest, data);
    DialogHandler::Command(change.serialize());
    WaitAndShot("Result_failed");

    // push destroy
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::_none, data);
    DialogHandler::Command(change.serialize());

    WaitLoop();
    Screens::Access()->Loop(); // for test only, this is very unsafe, do not use it like this anywhere else !!!
};

ScreenTestSelftest::ScreenTestSelftest()
    : AddSuperWindow<screen_t>()
    , header(this, string_view_utf8::MakeCPUFLASH((uint8_t *)"TEST of selftest dialogs"))
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes, string_view_utf8::MakeCPUFLASH((const uint8_t *)"back"))
    , btn_run(
          this, Rect16(10, 76, 220, 22), []() { SelftestTest(); }, string_view_utf8::MakeCPUFLASH((uint8_t *)"selftest test")) {
}
