#include <cstddef>
#include <unistd.h>

#include "screen_print_preview.hpp"
#include "log.h"
#include "gcode_file.h"
#include "marlin_client.hpp"
#include "filament_sensors_handler.hpp"
#include <stdarg.h>
#include "sound.hpp"
#include "DialogHandler.hpp"
#include "ScreenHandler.hpp"
#include "screen_printing.hpp"
#include "print_utils.hpp"
#include "client_response.hpp"
#include "printers.h"
#include "RAII.hpp"
#include "box_unfinished_selftest.hpp"
#include "window_msgbox_wrong_printer.hpp"

ScreenPrintPreview::ScreenPrintPreview()
    : gcode(GCodeInfo::getInstance())
    , gcode_description(this, gcode)
    , thumbnail(this, GuiDefaults::PreviewThumbnailRect)
    , phase(PhasesPrintPreview::_first) {

    assert(gcode.is_file_open() && "GCodeInfo must be initialized by guimain before ScreenPrintPreview is created");

    super::ClrMenuTimeoutClose();

    // title_text.set_font(GuiDefaults::FontBig); //TODO big font somehow does not work
    //  this MakeRAM is safe - gcode_file_name is set to vars->media_LFN, which is statically allocated in RAM
    title_text.SetText(string_view_utf8::MakeRAM((const uint8_t *)gcode.GetGcodeFilename()));

    CaptureNormalWindow(radio);
    ths = this;
}

ScreenPrintPreview::~ScreenPrintPreview() {
    GCodeInfo::getInstance().deinitFile();
    ths = nullptr;
}

// static variables and member functions
ScreenPrintPreview *ScreenPrintPreview::ths = nullptr;

ScreenPrintPreview *ScreenPrintPreview::GetInstance() { return ths; }

ScreenPrintPreview::UniquePtr ScreenPrintPreview::makeMsgBox(string_view_utf8 caption, string_view_utf8 text) {
    return make_static_unique_ptr<MsgBoxTitled>(&msgBoxMemSpace, GuiDefaults::RectScreenNoHeader, Responses_NONE, 0, nullptr, text, is_multiline::yes, caption, &png::warning_16x16, is_closed_on_click_t::no);
}

void ScreenPrintPreview::Change(fsm::BaseData data) {
    auto old_phase = phase;
    phase = GetEnumFromPhaseIndex<PhasesPrintPreview>(data.GetPhase());

    if (phase == old_phase)
        return;

    // need to call deleter before pointer is assigned, because new object is in same area of memory
    pMsgbox.reset();

    switch (phase) {
    case PhasesPrintPreview::main_dialog:
        break;
    case PhasesPrintPreview::unfinished_selftest:
        pMsgbox = makeMsgBox(_(labelWarning), _(txt_unfinished_selftest));
        break;
    case PhasesPrintPreview::new_firmware_available: {
        const auto version = GCodeInfo::getInstance().get_valid_printer_settings().latest_fw_version;
        static char str[] = "v00.00.000";
        snprintf(str, std::size(str), "%d.%d.%d", version.major, version.minor, version.patch);
        pMsgbox = makeMsgBox(_("New firmware available"), string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(str)));
        break;
    }
    case PhasesPrintPreview::wrong_printer:
    case PhasesPrintPreview::wrong_printer_abort:
        pMsgbox = make_static_unique_ptr<MsgBoxInvalidPrinter>(&msgBoxMemSpace, GuiDefaults::RectScreenNoHeader, _(labelWarning), &png::warning_16x16);
        break;
    case PhasesPrintPreview::filament_not_inserted:
        pMsgbox = makeMsgBox(_(labelWarning), _(txt_fil_not_detected));
        break;
    case PhasesPrintPreview::mmu_filament_inserted:
        pMsgbox = makeMsgBox(_(labelWarning), _(txt_fil_detected_mmu));
        break;
    case PhasesPrintPreview::wrong_filament:
        pMsgbox = makeMsgBox(_(labelWarning), _(txt_wrong_fil_type));
        break;
    }

    if (pMsgbox)
        pMsgbox->BindToFSM(phase);
}

void ScreenPrintPreview::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    if (event_in_progress)
        return;

    AutoRestore avoid_recursion(event_in_progress, true);
}
