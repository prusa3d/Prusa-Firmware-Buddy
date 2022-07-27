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
#include "print_utils.hpp"
#include "printers.h"

const uint16_t menu_icons[2] = {
    IDR_PNG_print_58px,
    IDR_PNG_stop_58px,
};

/// \returns true if filament is (finally) present or FS is disabled
static bool check_filament_presence(GCodeInfo &gcode) {
    // While in non-MMU2 mode perform a pre-print filament check.
    // While in MMU2 mode the operation is directly opposite
    // - the filament must NOT be present and the G-code specifies which filament shall be loaded after the start of the print.
    while (!FSensors_instance().CanStartPrint()) {
        bool has_mmu = FSensors_instance().HasMMU();
        Sound_Play(eSOUND_TYPE::SingleBeep);
        const PhaseResponses btns = has_mmu ? Responses_YesNo : Responses_YesNoIgnore;
        string_view_utf8 txt_fil_not_detected = has_mmu ? _("Filament detected. Unload filament now? Select NO to cancel.") : _("Filament not detected. Load filament now? Select NO to cancel, or IGNORE to disable the filament sensor and continue.");
        // this MakeRAM is safe - gcode.gcode_file_name is valid during the lifetime of the MsgBox
        switch (
#ifdef USE_ST7789
            MsgBoxWarning(txt_fil_not_detected, btns, 0, GuiDefaults::RectScreenNoHeader)
#else
            MsgBoxTitle(string_view_utf8::MakeRAM((const uint8_t *)gcode.GetGcodeFilename()), txt_fil_not_detected, btns, 0, GuiDefaults::RectScreenNoHeader)
#endif
        ) {
        case Response::Yes: //YES - load
            if (has_mmu) {
                PreheatStatus::DialogBlockingUnLoad(RetAndCool_t::Neither);
            } else {
                PreheatStatus::DialogBlockingLoad(RetAndCool_t::Return);
            }
            break;
        case Response::No: //NO - cancel
            return false;
        case Response::Ignore: //IGNORE - disable, outside MMU mode only
            FSensors_instance().Disable();
            return true;
        default:
            //should happen only if the message box was closed because flash was removed
            return false;
        }
    }
    return true;
}

static bool is_same(const char *curr_filament, const char (&filament_type)[GCodeInfo::filament_type_len]) {
    return strncmp(curr_filament, filament_type, GCodeInfo::filament_type_len) == 0;
}
static bool filament_known(const char *curr_filament) {
    return strncmp(curr_filament, "---", 3) != 0;
}

/// \returns true if filament has (finally) the correct type or the type is ignored
static bool check_filament_type(GCodeInfo &gcode) {
    for (const char *curr_filament = Filaments::Current().name;
         gcode.filament_described && filament_known(curr_filament) && !is_same(curr_filament, gcode.filament_type);
         curr_filament = Filaments::Current().name) {
        string_view_utf8 txt_wrong_fil_type = _("This G-CODE was set up for another filament type.");
        switch (MsgBoxWarning(txt_wrong_fil_type, Responses_ChangeIgnoreAbort, 0, GuiDefaults::RectScreenNoHeader)) {
        case Response::Change:
            PreheatStatus::DialogBlockingChangeLoad(RetAndCool_t::Return);
            break;
        case Response::Ignore:
            return true;
        case Response::Abort:
            return false;
        default:
            //should happen only if the message box was closed because flash was removed
            return false;
        }
    }
    return true;
}

/// \returns true if it's correct printer or printer type is ignored
static bool check_printer_type(GCodeInfo &gcode) {
    if (gcode.IsSettingsValid())
        return true;
    string_view_utf8 txt_wrong_printer_type = _("This G-CODE was set up for another printer type.");
    switch (MsgBoxWarning(txt_wrong_printer_type, Responses_IgnoreAbort, 0, GuiDefaults::RectScreenNoHeader)) {
    case Response::Abort:
        return false;
    case Response::Ignore:
        return true;
    default:
        //should happen only if the message box was closed because flash was removed
        return false;
    }
    return true;
}

static void print_button_pressed() {
    GCodeInfo &gcode = GCodeInfo::getInstance();
    if (!check_printer_type(gcode)
        || !check_filament_presence(gcode)
        || !check_filament_type(gcode)) {
        Sound_Play(eSOUND_TYPE::SingleBeep);
        Screens::Access()->Close();
        return;
    }

    print_begin(gcode.GetGcodeFilepath());
}

screen_print_preview_data_t::screen_print_preview_data_t()
    : AddSuperWindow<screen_t>()
    , title_text(this, Rect16(PADDING, PADDING, SCREEN_WIDTH - 2 * PADDING, TITLE_HEIGHT))
    , print_button(this, Rect16(PADDING, SCREEN_HEIGHT - PADDING - LINE_HEIGHT - 64, 64, 64), IDR_PNG_print_58px, print_button_pressed)
    , print_label(this, Rect16(PADDING, SCREEN_HEIGHT - PADDING - LINE_HEIGHT, 64, LINE_HEIGHT), is_multiline::no)
    , back_button(this, Rect16(SCREEN_WIDTH - PADDING - 64, SCREEN_HEIGHT - PADDING - LINE_HEIGHT - 64, 64, 64), IDR_PNG_back_32px, []() { Screens::Access()->Close(); })
    , back_label(this, Rect16(SCREEN_WIDTH - PADDING - 64, SCREEN_HEIGHT - PADDING - LINE_HEIGHT, 64, LINE_HEIGHT), is_multiline::no)
    , thumbnail(this, GuiDefaults::PreviewThumbnailRect)
    , gcode(GCodeInfo::getInstance())
    , gcode_description(this, gcode) {

    marlin_set_print_speed(100);

    super::ClrMenuTimeoutClose();
    // Title
    title_text.font = resource_font(IDR_FNT_BIG);
    // this MakeRAM is safe - gcode_file_name is set to vars->media_LFN, which is statically allocated in RAM
    title_text.SetText(string_view_utf8::MakeRAM((const uint8_t *)gcode.GetGcodeFilename()));

    print_label.SetText(_("Print"));
    print_label.SetAlignment(Align_t::Center());
    print_label.font = resource_font(IDR_FNT_SMALL);

    back_label.SetText(_("Back"));
    back_label.SetAlignment(Align_t::Center());
    back_label.font = resource_font(IDR_FNT_SMALL);
}

bool screen_print_preview_data_t::gcode_file_exists() {
    return access(gcode.GetGcodeFilepath(), F_OK) == 0;
}

void screen_print_preview_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    // In case the file is no longer present, close this screen.
    // (Most likely because of usb flash drive disconnection).
    if (event == GUI_event_t::LOOP && !gcode_file_exists()) {
        Screens::Access()->Close(); //if an dialog is openned, it will be closed first
        return;
    }
    SuperWindowEvent(sender, event, param);
}
