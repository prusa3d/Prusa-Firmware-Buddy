/**
 * @file screen_test_load.cpp
 */

#include "screen_test_load.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "DialogHandler.hpp"
#include "fsm_loadunload_type.hpp"
#include "error_codes_mmu.hpp"
#include "../../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"
#include "../../../src/mmu2/mmu2_error_converter.h"
#include <option/has_mmu2.h>

static void WaitLoop() {
    static constexpr uint32_t switch_period = 2048;
    uint32_t switch_time = gui::GetTick();
    while ((gui::GetTick() - switch_time) < switch_period) {
        gui::TickLoop();
        gui_loop();
        DialogHandler::Access().Loop();
    }
}

static void set_state(PhasesLoadUnload phase, uint8_t progress) {
    ProgressSerializerLoadUnload serializer(LoadUnloadMode::Change, progress);
    fsm::Change change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(phase), serializer.Serialize()));
    DialogHandler::Command(change.serialize());
}

static void LoadUnloadTest() {
    uint8_t progress = 0;
    fsm::Change change(fsm::QueueIndex::q0);
    fsm::BaseData base_data;
#if HAS_MMU2()
    for (PhasesLoadUnload i = PhasesLoadUnload::_first; i < PhasesLoadUnload::MMU_ERRWaitingForUser; i = PhasesLoadUnload(int(i) + 1)) {
        set_state(i, progress);

        WaitLoop();

        progress += 10;
        progress %= 101; // limit to 100
    }

    // push change
    // focus must be on middle button
    fsm::PhaseData data = fsm::PointerSerializer<MMU2::MMUErrDesc>(MMU2::ConvertMMUErrorCode(ErrorCode::FINDA_DIDNT_SWITCH_OFF)).Serialize();
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data));
    DialogHandler::Command(change.serialize());

    WaitLoop();

    // push change
    // only middle button, it ha focus
    data = fsm::PointerSerializer<MMU2::MMUErrDesc>(MMU2::ConvertMMUErrorCode(ErrorCode::MMU_NOT_RESPONDING)).Serialize();
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data));
    DialogHandler::Command(change.serialize());

    WaitLoop();

    // push change
    // focus must be on middle button
    data = fsm::PointerSerializer<MMU2::MMUErrDesc>(MMU2::ConvertMMUErrorCode(ErrorCode::FINDA_DIDNT_SWITCH_ON)).Serialize();
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data));
    DialogHandler::Command(change.serialize());

    WaitLoop();

    // push change
    // focus must be on button from previous state
    data = fsm::PointerSerializer<MMU2::MMUErrDesc>(MMU2::ConvertMMUErrorCode(ErrorCode::FSENSOR_DIDNT_SWITCH_ON)).Serialize();
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::Load_unload, fsm::BaseData(GetPhaseIndex(PhasesLoadUnload::MMU_ERRWaitingForUser), data));
    DialogHandler::Command(change.serialize());

    WaitLoop();

    for (PhasesLoadUnload i = PhasesLoadUnload(int(PhasesLoadUnload::MMU_ERRWaitingForUser) + 1); i <= PhasesLoadUnload::_last; i = PhasesLoadUnload(int(i) + 1)) {
        set_state(i, progress);

        WaitLoop();

        progress += 10;
        progress %= 110;
    }
#else
    for (PhasesLoadUnload i = PhasesLoadUnload::_first; i <= PhasesLoadUnload::_last; i = PhasesLoadUnload(int(i) + 1)) {
        set_state(i, progress);

        WaitLoop();

        progress += 10;
        progress %= 110;
    }
#endif // HAS_MMU2()

    // close
    change = fsm::Change(fsm::QueueIndex::q0, ClientFSM::_none, base_data);
    DialogHandler::Command(change.serialize());
};

ScreenTestMMU::ScreenTestMMU()
    : AddSuperWindow<screen_t>()
    , header(this, string_view_utf8::MakeCPUFLASH((uint8_t *)"TEST of load dialog"))
    , back(this, Rect16(10, 54, 220, 22), is_multiline::no, is_closed_on_click_t::yes, string_view_utf8::MakeCPUFLASH((const uint8_t *)"back"))
    , tst_load(
          this, Rect16(10, 76, 220, 22), []() { LoadUnloadTest(); }, string_view_utf8::MakeCPUFLASH((uint8_t *)"load test")) {
}
