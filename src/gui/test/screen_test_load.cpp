/**
 * @file screen_test_load.cpp
 */

#include "screen_test_load.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "DialogHandler.hpp"

static void WaitLoop() {
    static constexpr uint32_t switch_period = 2048;
    uint32_t switch_time = gui::GetTick();
    while ((gui::GetTick() - switch_time) < switch_period) {
        gui::TickLoop();
        gui_loop();
        DialogHandler::Access().Loop();
    }
}

static void LoadUnloadTest() {
    fsm::variant_t var;
    fsm::PhaseData data;
    //push create
    var = fsm::variant_t(fsm::create_t(ClientFSM::Load_unload, 0));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

    //push change
    var = fsm::variant_t(fsm::change_t(ClientFSM::Load_unload, fsm::BaseData(1, { 0, 0, 0, 25 })));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

    //push change
    var = fsm::variant_t(fsm::change_t(ClientFSM::Load_unload, fsm::BaseData(3, { 0, 0, 0, 75 })));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

    //push destroy
    var = fsm::variant_t(fsm::destroy_t(ClientFSM::Load_unload));
    DialogHandler::Command(var.u32, var.u16);
};

ScreenTestMMU::ScreenTestMMU()
    : AddSuperWindow<screen_t>()
    , header(this, string_view_utf8::MakeCPUFLASH((uint8_t *)"TEST of load dialog"))
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes, string_view_utf8::MakeCPUFLASH((const uint8_t *)"back"))
    , tst_load(
          this, Rect16(10, 76, 220, 22), []() { LoadUnloadTest(); }, string_view_utf8::MakeCPUFLASH((uint8_t *)"load test")) {
}
