/**
 * @file screen_test_load.cpp
 */

#include "screen_test_load.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "DialogHandler.hpp"
#include "../../../lib/Prusa-Error-Codes/04_MMU/errors_list.h"
#include "../../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"
#include "../../../src/mmu2/mmu2_error_converter.h"

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
    //push create
    var = fsm::variant_t(fsm::create_t(ClientFSM::Load_unload, 0));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

    //push change
    var = fsm::variant_t(fsm::change_t(ClientFSM::Load_unload, fsm::BaseData(1, { 0, 0, 0, 25 })));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

#if HAS_MMU2
    fsm::PhaseData data;
    //push change
    //focus must be on middle button
    data = fsm::PointerSerializer<MMU2::MMUErrorDesc>(MMU2::ConvertMMUErrorCode(uint16_t(ErrorCode::FINDA_DIDNT_SWITCH_OFF))).Serialize();
    var = fsm::variant_t(fsm::change_t(ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data)));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

    //push change
    //only middle button, it ha focus
    data = fsm::PointerSerializer<MMU2::MMUErrorDesc>(MMU2::ConvertMMUErrorCode(uint16_t(ErrorCode::MMU_NOT_RESPONDING))).Serialize();
    var = fsm::variant_t(fsm::change_t(ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data)));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

    //push change
    //focus must be on middle button
    data = fsm::PointerSerializer<MMU2::MMUErrorDesc>(MMU2::ConvertMMUErrorCode(uint16_t(ErrorCode::FINDA_DIDNT_SWITCH_ON))).Serialize();
    var = fsm::variant_t(fsm::change_t(ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data)));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();

    //push change
    //focus must be on button from previous state
    data = fsm::PointerSerializer<MMU2::MMUErrorDesc>(MMU2::ConvertMMUErrorCode(uint16_t(ErrorCode::FSENSOR_DIDNT_SWITCH_ON))).Serialize();
    var = fsm::variant_t(fsm::change_t(ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data)));
    DialogHandler::Command(var.u32, var.u16);

    WaitLoop();
#endif // HAS_MMU2

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
