#include "screen_print_preview.hpp"
#include "dbg.h"
#include "marlin_client.h"
#include "resource.h"
#include "window_dlg_load_unload.hpp"
#include "filament_sensor.hpp"
#include <stdarg.h>
#include "sound.hpp"
#include "DialogHandler.hpp"
#include "ScreenHandler.hpp"
#include "print_utils.hpp"
#include "printers.h"
#include "gcode_file.h"

#define DBG _dbg0

const uint16_t menu_icons[2] = {
    IDR_PNG_print_58px,
    IDR_PNG_stop_58px,
};

size_t description_line_t::title_width(string_view_utf8 *title_str) {
    return title_str->computeNumUtf8CharsAndRewind() * resource_font(IDR_FNT_SMALL)->w;
}

size_t description_line_t::value_width(string_view_utf8 *title_str) {
    return SCREEN_WIDTH - PADDING * 2 - title_width(title_str) - 1;
}

description_line_t::description_line_t(window_frame_t *frame, bool has_preview_thumbnail, size_t row, string_view_utf8 title_str, const char *value_fmt, ...)
    : title(frame, Rect16(PADDING, calculate_y(has_preview_thumbnail, row), title_width(&title_str), LINE_HEIGHT), is_multiline::no)
    , value(frame, Rect16(SCREEN_WIDTH - PADDING - value_width(&title_str), calculate_y(has_preview_thumbnail, row), value_width(&title_str), LINE_HEIGHT), is_multiline::no) {
    title.SetText(title_str);
    title.SetAlignment(Align_t::LeftBottom());
    title.SetPadding({ 0, 0, 0, 0 });
    title.font = resource_font(IDR_FNT_SMALL);
    title.SetTextColor(COLOR_GRAY);

    va_list args;
    va_start(args, value_fmt);
    vsnprintf(value_buffer, sizeof(value_buffer), value_fmt, args);
    va_end(args);
    // this MakeRAM is safe - value_buffer is allocated in RAM for the lifetime of line
    value.SetText(string_view_utf8::MakeRAM((const uint8_t *)value_buffer));
    value.SetAlignment(Align_t::RightBottom());
    value.SetPadding({ 0, 0, 0, 0 });
    value.font = resource_font(IDR_FNT_SMALL);
}

GCodeInfoWithDescription::GCodeInfoWithDescription(window_frame_t *frame, GCodeInfo &gcode)
    : description_lines {
        gcode.printing_time[0] ? description_line_t(frame, gcode.has_preview_thumbnail, 0, _("Print Time"), "%s", gcode.printing_time) : description_line_t(frame, gcode.has_preview_thumbnail, 0, _("Print Time"), "unknown"),
        gcode.has_preview_thumbnail ? description_line_t(frame, gcode.has_preview_thumbnail, 1, _("Material"), "%s/%u g/%0.2f m", gcode.filament_type, gcode.filament_used_g, (double)((float)gcode.filament_used_mm / 1000.0F)) : description_line_t(frame, gcode.has_preview_thumbnail, 1, _("Material"), "%s", gcode.filament_type),
        { frame, gcode.has_preview_thumbnail, 2, _("Used Filament"), "%.2f m", (double)((float)gcode.filament_used_mm / 1000.0F) },
        { frame, gcode.has_preview_thumbnail, 3, string_view_utf8::MakeNULLSTR(), "%.0f g", (double)gcode.filament_used_g }
    } {
    if (gcode.has_preview_thumbnail || !gcode.filament_type[0]) {
        description_lines[2].value.Hide();
        description_lines[2].title.Hide();
        description_lines[2].value.Validate(); // == do not redraw
        description_lines[2].title.Validate(); // == do not redraw
    }
    if (gcode.has_preview_thumbnail || !(gcode.filament_used_mm && gcode.filament_used_g)) {
        description_lines[3].value.Hide();
        description_lines[3].title.Hide();
        description_lines[3].value.Validate(); // == do not redraw
        description_lines[3].title.Validate(); // == do not redraw
    }
}

static void print_button_press() {
    bool approved = true;
    GCodeInfo &gcode = GCodeInfo::getInstance();
    if (!gcode.IsSettingsValid()) {
        switch (MsgBoxTitle(_("WARNING:"), _("This G-CODE was set up for another printer type."),
            Responses_OkCancel, 0, GuiDefaults::RectScreenBody)) {
        case Response::Ok:
            break;
        case Response::Cancel:
            Sound_Play(eSOUND_TYPE::SingleBeep);
            approved = false;
            break;
        default:
            break;
        }
    }
    if (approved) {
        print_begin(gcode.GetGcodeFilepath());
    }
}

screen_print_preview_data_t::screen_print_preview_data_t()
    : AddSuperWindow<screen_t>()
    , title_text(this, Rect16(PADDING, PADDING, SCREEN_WIDTH - 2 * PADDING, TITLE_HEIGHT))
    , print_button(this, Rect16(PADDING, SCREEN_HEIGHT - PADDING - LINE_HEIGHT - 64, 64, 64), IDR_PNG_print_58px, print_button_press)
    , print_label(this, Rect16(PADDING, SCREEN_HEIGHT - PADDING - LINE_HEIGHT, 64, LINE_HEIGHT), is_multiline::no)
    , back_button(this, Rect16(SCREEN_WIDTH - PADDING - 64, SCREEN_HEIGHT - PADDING - LINE_HEIGHT - 64, 64, 64), IDR_PNG_back_32px, []() { Screens::Access()->Close(); })
    , back_label(this, Rect16(SCREEN_WIDTH - PADDING - 64, SCREEN_HEIGHT - PADDING - LINE_HEIGHT, 64, LINE_HEIGHT), is_multiline::no)
    , thumbnail(this, GuiDefaults::PreviewThumbnailRect)
    , gcode(GCodeInfo::getInstance())
    , ignore_wrong_filament(!gcode.filament_described)
    , gcode_description(this, gcode) {

    marlin_set_print_speed(100);

    suppress_draw = false;

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
    FILINFO finfo = { 0 };
    return f_stat(gcode.GetGcodeFilepath(), &finfo) == FR_OK;
}

//FIXME simple solution not to brake functionality before release
//rewrite later
void screen_print_preview_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    // In case the file is no longer present, close this screen.
    // (Most likely because of usb flash drive disconnection).
    if (!gcode_file_exists()) {
        Screens::Access()->Close(); //if an dialog is openned, it will be closed first
        return;
    }

    if (!suppress_draw && FS_instance().DidRunOut()) {
        suppress_draw = true;
        Sound_Play(eSOUND_TYPE::SingleBeep);
        const PhaseResponses btns = { Response::Yes, Response::No, Response::Ignore, Response::_none };
        // this MakeRAM is safe - vars->media_LFN is statically allocated (even though it may not be obvious at the first look)
        switch (MsgBoxTitle(string_view_utf8::MakeRAM((const uint8_t *)gcode.GetGcodeFilename()),
            _("Filament not detected. Load filament now? Select NO to cancel, or IGNORE to disable the filament sensor and continue."),
            btns, 0, GuiDefaults::RectScreenBody)) {
        case Response::Yes: //YES - load
            PreheatStatus::DialogBlocking(PreheatMode::Load, RetAndCool_t::Return);
            break;
        case Response::No: //NO - cancel
            Screens::Access()->Close();
            return;
        case Response::Ignore: //IGNORE - disable
            FS_instance().Disable();
            break;
        default:
            break;
        }

        const char *curr_filament = Filaments::Current().name;
        if (!ignore_wrong_filament && strncmp(curr_filament, "---", 3) != 0 && strncmp(curr_filament, gcode.filament_type, sizeof(gcode.filament_type)) != 0) {
            switch (MsgBoxTitle(_("WARNING:"), _("This G-CODE was set up for another filament type."),
                Responses_ChangeIgnoreCancel, 0, GuiDefaults::RectScreen)) {
            case Response::Change:
                PreheatStatus::DialogBlocking(PreheatMode::Change_phase1, RetAndCool_t::Return);
                break;
            case Response::Ignore:
                ignore_wrong_filament = true;
                break;
            case Response::Cancel:
                Sound_Play(eSOUND_TYPE::SingleBeep);
                Screens::Access()->Close();
                break;
            default:
                break;
            }
        }

        suppress_draw = false;
        //window_draw(id);
    }

    SuperWindowEvent(sender, event, param);
}
