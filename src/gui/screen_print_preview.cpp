#include <unistd.h>

#include "screen_print_preview.hpp"
#include "log.h"
#include "gcode_file.h"
#include "marlin_client.h"
#include "resource.h"
#include "window_dlg_load_unload.hpp"
#include "filament_sensor_api.hpp"
#include <stdarg.h>
#include "sound.hpp"
#include "DialogHandler.hpp"
#include "ScreenHandler.hpp"
#include "screen_printing.hpp"
#include "print_utils.hpp"
#include "client_response.hpp"
#include "printers.h"

static GCodeInfo &gcode_init() {
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_FILENAME) | MARLIN_VAR_MSK(MARLIN_VAR_FILEPATH));
    GCodeInfo::getInstance().initFile(GCodeInfo::GI_INIT_t::PREVIEW);
    return GCodeInfo::getInstance();
}

ScreenPrintPreview::ScreenPrintPreview()
    : title_text(this, Rect16(PADDING, PADDING, display::GetW() - 2 * PADDING, TITLE_HEIGHT))
    , radio(this, GuiDefaults::GetIconnedButtonRect(GetRect()), ClientResponses::GetResponses(PhasesPrintPreview::main_dialog))
    , gcode(gcode_init())
    , gcode_description(this, gcode)
    , thumbnail(this, GuiDefaults::PreviewThumbnailRect)
    , phase(PhasesPrintPreview::_first) {

    super::ClrMenuTimeoutClose();

    //title_text.font = GuiDefaults::FontBig; //TODO big font somehow does not work
    // this MakeRAM is safe - gcode_file_name is set to vars->media_LFN, which is statically allocated in RAM
    title_text.SetText(string_view_utf8::MakeRAM((const uint8_t *)gcode.GetGcodeFilename()));

    radio.SetHasIcon();
    radio.SetBlackLayout(); // non iconned buttons have orange background
    radio.SetBtnCount(2);
    ths = this;
}

ScreenPrintPreview::~ScreenPrintPreview() {
    ths = nullptr;
}

//static variables and member functions
ScreenPrintPreview *ScreenPrintPreview::ths = nullptr;

ScreenPrintPreview *ScreenPrintPreview::GetInstance() { return ths; }

/*
TODO
Sound_Play(eSOUND_TYPE::SingleBeep); in case of failure of print action
*/
void ScreenPrintPreview::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        Response response = radio.Click();
        marlin_FSM_response(phase, response);
        break;
    }
    case GUI_event_t::CHILD_CLICK: {
        event_conversion_union un;
        un.pvoid = param;
        Response response = un.response;
        marlin_FSM_response(phase, response);
        break;
    }
    case GUI_event_t::ENC_UP:
        ++radio;
        break;
    case GUI_event_t::ENC_DN:
        --radio;
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

ScreenPrintPreview::UniquePtr ScreenPrintPreview::makeMsgBox(const PhaseResponses &resp, string_view_utf8 caption, string_view_utf8 text) {
    return make_static_unique_ptr<MsgBoxTitled>(&msgBoxMemSpace, GuiDefaults::RectScreenNoHeader, resp, 0, nullptr, text, is_multiline::yes, caption, PNG::warning_16x16, is_closed_on_click_t::no);
}

void ScreenPrintPreview::Change(fsm::BaseData data) {
    auto old_phase = phase;
    phase = GetEnumFromPhaseIndex<PhasesPrintPreview>(data.GetPhase());

    if (phase == old_phase)
        return;

    //need to call deleter before pointer is assigned, because new object is in same area of memory
    pMsgbox.reset();

    switch (phase) {
    case PhasesPrintPreview::main_dialog:
        break;
    case PhasesPrintPreview::wrong_printer:
        pMsgbox = makeMsgBox(Responses_IgnoreAbort, _(labelWarning), _(txt_wrong_printer_type));
        break;
    case PhasesPrintPreview::filament_not_inserted:
        pMsgbox = makeMsgBox(Responses_YesNoFSDisable, _(labelWarning), _(txt_fil_not_detected));
        break;
    case PhasesPrintPreview::mmu_filament_inserted:
        pMsgbox = makeMsgBox(Responses_YesNo, _(labelWarning), _(txt_fil_detected_mmu));
        break;
    case PhasesPrintPreview::wrong_filament:
        pMsgbox = makeMsgBox(Responses_ChangeOkAbort, _(labelWarning), _(txt_wrong_fil_type));
        break;
    }
}
