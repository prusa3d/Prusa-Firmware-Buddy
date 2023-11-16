#include <cstddef>
#include <unistd.h>

#include "screen_print_preview.hpp"
#include "log.h"
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
#include <option/has_toolchanger.h>
#include <option/has_mmu2.h>
#include <device/board.h>
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

ScreenPrintPreview::ScreenPrintPreview()
    : gcode(GCodeInfo::getInstance())
    , gcode_description(this)
    , thumbnail(this, GuiDefaults::PreviewThumbnailRect) {

    super::ClrMenuTimeoutClose();

    // title_text.set_font(GuiDefaults::FontBig); //TODO big font somehow does not work
    //  this MakeRAM is safe - gcode_file_name is set to vars->media_LFN, which is statically allocated in RAM
    title_text.SetText(string_view_utf8::MakeRAM((const uint8_t *)gcode.GetGcodeFilename()));

    CaptureNormalWindow(radio);
    ths = this;
}

ScreenPrintPreview::~ScreenPrintPreview() {
    ths = nullptr;
}

// static variables and member functions
ScreenPrintPreview *ScreenPrintPreview::ths = nullptr;

ScreenPrintPreview *ScreenPrintPreview::GetInstance() { return ths; }

void ScreenPrintPreview::Change(fsm::BaseData data) {
    auto old_phase = phase;
    phase = GetEnumFromPhaseIndex<PhasesPrintPreview>(data.GetPhase());

    if (phase == old_phase) {
        return;
    }

    // need to call deleter before pointer is assigned, because new object is in same area of memory
    pMsgbox.reset();

    if (phase != PhasesPrintPreview::main_dialog) {
        hide_main_dialog();
    }
#if HAS_TOOLCHANGER() || HAS_MMU2()
    if (phase != PhasesPrintPreview::tools_mapping) {
        spool_join.reset();
        header.hide_bed_info();
    }
#endif

    const auto makeMsgBox = [this](string_view_utf8 caption, string_view_utf8 text, const img::Resource &icon = img::warning_16x16) {
        return make_static_unique_ptr<MsgBoxTitled>(&msgBoxMemSpace, GuiDefaults::RectScreenNoHeader, Responses_NONE, 0, nullptr, text, is_multiline::yes, caption, &icon, is_closed_on_click_t::no);
    };
    const auto makeMsgBoxWait = [this](string_view_utf8 text) {
        return make_static_unique_ptr<MsgBoxIconnedWait>(&msgBoxMemSpace, GuiDefaults::RectScreenNoHeader, Responses_NONE, 0, nullptr, text, is_multiline::yes);
    };

    switch (phase) {

    case PhasesPrintPreview::loading:
        pMsgbox = makeMsgBoxWait(_("Loading..."));
        break;

    case PhasesPrintPreview::download_wait:
        pMsgbox = makeMsgBoxWait(_("Downloading..."));
        break;

    case PhasesPrintPreview::main_dialog:
        gcode_description.update(gcode);
        assert(gcode.is_loaded() && "GCodeInfo must be initialized before ScreenPrintPreview is created");
        show_main_dialog();
        break;

    case PhasesPrintPreview::unfinished_selftest:
        pMsgbox = makeMsgBox(_(labelWarning), _(txt_unfinished_selftest));
        break;

    case PhasesPrintPreview::new_firmware_available: {
        const auto version = GCodeInfo::getInstance().get_valid_printer_settings().latest_fw_version;
        pMsgbox = makeMsgBox(_(txt_new_fw_available), string_view_utf8::MakeRAM(reinterpret_cast<const uint8_t *>(version)));
        break;
    }

    case PhasesPrintPreview::wrong_printer:
    case PhasesPrintPreview::wrong_printer_abort:
        pMsgbox = make_static_unique_ptr<MsgBoxInvalidPrinter>(&msgBoxMemSpace, GuiDefaults::RectScreenNoHeader, _(labelWarning), &img::warning_16x16);
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

    case PhasesPrintPreview::file_error:
        pMsgbox = makeMsgBox(_("File error"), _(gcode.error_str()), img::error_16x16);
        break;

    case PhasesPrintPreview::tools_mapping:
        show_tools_mapping();
        break;
    }

    if (pMsgbox) {
        pMsgbox->BindToFSM(phase);
    }
}

void ScreenPrintPreview::hide_main_dialog() {
    for (auto &line : gcode_description.description_lines) {
        line.title.Hide();
        line.value.Hide();
    }

    thumbnail.Hide();
    radio.Hide();
    title_text.Hide();
}

void ScreenPrintPreview::show_main_dialog() {
    for (auto &line : gcode_description.description_lines) {
        line.title.Show();
        line.value.Show();
    }

    thumbnail.Show();
    radio.Show();
    title_text.Show();
    CaptureNormalWindow(radio);
#if BOARD_IS_XBUDDY or BOARD_IS_XLBUDDY
    header.SetText(_("PRINT"));
#endif
}

void ScreenPrintPreview::show_tools_mapping() {
#if HAS_TOOLCHANGER() || HAS_MMU2()
    #if HAS_MMU2()
    if (!MMU2::mmu2.Enabled()) {
        return;
    }
    #endif

    tools_mapping = make_static_unique_ptr<ToolsMappingBody>(&msgBoxMemSpace, this, gcode);
    CaptureNormalWindow(*tools_mapping);
    tools_mapping->Show();
    tools_mapping->Invalidate();

    #if BOARD_IS_XBUDDY or BOARD_IS_XLBUDDY
    header.SetText(_("TOOLS MAPPING"));
    #endif

    header.show_bed_info();
#endif
}

void ScreenPrintPreview::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    // Catch event when USB is removed
    if (event == GUI_event_t::MEDIA) {
        const MediaState_t media_state = MediaState_t(reinterpret_cast<int>(param));
        if (media_state == MediaState_t::removed || media_state == MediaState_t::error) {
            marlin_client::print_abort(); // Abort print from marlin_server and close printing screens
        }
    }
}
