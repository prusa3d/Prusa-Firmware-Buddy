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

static void LoadUnloadTest() {
    mkdir("/usb/selftest", 777);
    fsm::variant_t var;
    fsm::BaseData data;
    //push create
    var = fsm::variant_t(fsm::create_t(ClientFSM::Selftest, 0));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();
    Screens::Access()->Loop(); // for test only, this is very unsafe, do not use it like this anywhere else !!!

    for (PhasesSelftest i = PhasesSelftest::_first; int(i) <= int(PhasesSelftest::_last); i = PhasesSelftest(int(i) + 1)) {
        data.SetPhase(int(i) - int(PhasesSelftest::_first));

        //push change
        var = fsm::variant_t(fsm::change_t(ClientFSM::Selftest, data));
        DialogHandler::Command(var.u32, var.u16);

        WaitAndShot(get_selftest_state_name(i));
    }

    //push destroy
    var = fsm::variant_t(fsm::destroy_t(ClientFSM::Selftest));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();
    Screens::Access()->Loop(); // for test only, this is very unsafe, do not use it like this anywhere else !!!
};

ScreenTestSelftest::ScreenTestSelftest()
    : AddSuperWindow<screen_t>()
    , header(this, string_view_utf8::MakeCPUFLASH((uint8_t *)"TEST of selftest dialogs"))
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes, string_view_utf8::MakeCPUFLASH((const uint8_t *)"back"))
    , btn_run(
          this, Rect16(10, 76, 220, 22), []() { LoadUnloadTest(); }, string_view_utf8::MakeCPUFLASH((uint8_t *)"selftest test")) {
}
