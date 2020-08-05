#include "screen_print_preview.hpp"
#include "dbg.h"
#include "gcode_file.h"
#include "marlin_client.h"
#include "resource.h"
#include "window_dlg_load_unload.h"
#include "filament_sensor.h"
#include <stdarg.h>
#include "sound.hpp"
#include "DialogHandler.hpp"
#include "ScreenHandler.hpp"
#include "print_utils.hpp"

#define DBG _dbg0

static const char *gcode_file_name = NULL;
static const char *gcode_file_path = NULL;

const uint16_t menu_icons[2] = {
    IDR_PNG_menu_icon_print,
    IDR_PNG_menu_icon_stop,
};

void screen_print_preview_data_t::SetGcodeFilepath(const char *fpath) {
    gcode_file_path = fpath;
}

const char *screen_print_preview_data_t::GetGcodeFilepath() {
    return gcode_file_path;
}

void screen_print_preview_data_t::SetGcodeFilename(const char *fname) {
    gcode_file_name = fname;
}

size_t description_line_t::title_width(string_view_utf8 *title_str) {
    return title_str->computeNumUtf8CharsAndRewind() * resource_font(IDR_FNT_SMALL)->w;
}

size_t description_line_t::value_width(string_view_utf8 *title_str) {
    return SCREEN_WIDTH - PADDING * 2 - title_width(title_str) - 1;
}

description_line_t::description_line_t(window_frame_t *frame, bool has_thumbnail, size_t row, string_view_utf8 title_str, const char *value_fmt, ...)
    : title(frame, rect_ui16(PADDING, calculate_y(has_thumbnail, row), title_width(&title_str), LINE_HEIGHT))
    , value(frame, rect_ui16(SCREEN_WIDTH - PADDING - value_width(&title_str), calculate_y(has_thumbnail, row), value_width(&title_str), LINE_HEIGHT))

{
    title.SetText(title_str);
    title.SetAlignment(ALIGN_LEFT_BOTTOM);
    title.SetPadding({ 0, 0, 0, 0 });
    title.font = resource_font(IDR_FNT_SMALL);

    va_list args;
    va_start(args, value_fmt);
    vsnprintf(value_buffer, sizeof(value_buffer), value_fmt, args);
    va_end(args);
    // this MakeRAM is safe - value_buffer is allocated in RAM for the lifetime of line
    value.SetText(string_view_utf8::MakeRAM((const uint8_t *)value_buffer));
    value.SetAlignment(ALIGN_RIGHT_BOTTOM);
    value.SetPadding({ 0, 0, 0, 0 });
    value.font = resource_font(IDR_FNT_SMALL);
}

GCodeInfo::GCodeInfo() {
    memset(&file, 1, sizeof(FIL));
    file_opened = false;
    has_thumbnail = false;

    // try to open the file first
    if (!gcode_file_path || f_open(&file, gcode_file_path, FA_READ) != FR_OK) {
        return;
    }
    file_opened = true;

    // thumbnail presence check
    {
        FILE f = { 0 };
        if (f_gcode_thumb_open(&f, &file) == 0) {
            char buffer;
            has_thumbnail = fread((void *)&buffer, 1, 1, &f) > 0;
            f_gcode_thumb_close(&f);
        }
    }

    // find printing time and filament information
    printing_time[0] = 0;
    filament_type[0] = 0;
    filament_used_mm = 0;
    filament_used_g = 0;
    const unsigned search_last_x_bytes = 10000;
    FSIZE_t filesize = f_size(&file);
    f_lseek(&file, filesize > search_last_x_bytes ? filesize - search_last_x_bytes : 0);
    char name_buffer[64];
    char value_buffer[32];
    while (f_gcode_get_next_comment_assignment(
        &file, name_buffer, sizeof(name_buffer), value_buffer,
        sizeof(value_buffer))) {

#define name_equals(str) (!strncmp(name_buffer, str, sizeof(name_buffer)))

        if (name_equals("estimated printing time (normal mode)")) {
            snprintf(printing_time, sizeof(printing_time),
                "%s", value_buffer);
        } else if (name_equals("filament_type")) {
            snprintf(filament_type, sizeof(filament_type),
                "%s", value_buffer);
        } else if (name_equals("filament used [mm]")) {
            sscanf(value_buffer, "%u", &filament_used_mm);
        } else if (name_equals("filament used [g]")) {
            sscanf(value_buffer, "%u", &filament_used_g);
        }
    }
}

GCodeInfoWithDescription::GCodeInfoWithDescription(window_frame_t *frame)
    : description_lines {
        printing_time[0] ? description_line_t(frame, has_thumbnail, 0, _("Print Time"), "%s", printing_time) : description_line_t(frame, has_thumbnail, 0, _("Print Time"), "unknown"),
        has_thumbnail ? description_line_t(frame, has_thumbnail, 1, _("Material"), "%s/%u g/%0.2f m", filament_type, filament_used_g, (double)((float)filament_used_mm / 1000.0F)) : description_line_t(frame, has_thumbnail, 1, _("Material"), "%s", filament_type),
        { frame, has_thumbnail, 2, _("Used Filament"), "%.2f m", (double)((float)filament_used_mm / 1000.0F) },
        { frame, has_thumbnail, 3, string_view_utf8::MakeNULLSTR(), "%.0f g", (double)filament_used_g }
    } {
    if (has_thumbnail || !filament_type[0]) {
        description_lines[2].value.Hide();
        description_lines[2].title.Hide();
    }
    if (has_thumbnail || !(filament_used_mm && filament_used_g)) {
        description_lines[3].value.Hide();
        description_lines[3].title.Hide();
    }
}

screen_print_preview_data_t::screen_print_preview_data_t()
    : window_frame_t()
    , title_text(this, rect_ui16(PADDING, PADDING, SCREEN_WIDTH - 2 * PADDING, TITLE_HEIGHT))
    , print_button(this, rect_ui16(PADDING, SCREEN_HEIGHT - PADDING - LINE_HEIGHT - 64, 64, 64), IDR_PNG_menu_icon_print, []() { print_begin(screen_print_preview_data_t::GetGcodeFilepath()); })
    , print_label(this, rect_ui16(PADDING, SCREEN_HEIGHT - PADDING - LINE_HEIGHT, 64, 64))
    , back_button(this, rect_ui16(SCREEN_WIDTH - PADDING - 64, SCREEN_HEIGHT - PADDING - LINE_HEIGHT - 64, 64, 64), IDR_PNG_menu_icon_back, []() { Screens::Access()->Close(); })
    , back_label(this, rect_ui16(SCREEN_WIDTH - PADDING - 64, SCREEN_HEIGHT - PADDING - LINE_HEIGHT, 64, 64))
    , gcode(this)
    , redraw_thumbnail(gcode.has_thumbnail) {
    marlin_set_print_speed(100);

    // Title
    title_text.font = resource_font(IDR_FNT_BIG);
    // this MakeRAM is safe - gcode_file_name is set to vars->media_LFN, which is statically allocated in RAM
    title_text.SetText(string_view_utf8::MakeRAM((const uint8_t *)gcode_file_name));

    print_label.SetText(_("Print"));
    print_label.SetAlignment(ALIGN_CENTER);
    print_label.font = resource_font(IDR_FNT_SMALL);

    back_label.SetText(_("Back"));
    back_label.SetAlignment(ALIGN_CENTER);
    back_label.font = resource_font(IDR_FNT_SMALL);
}

screen_print_preview_data_t::~screen_print_preview_data_t() {
    if (gcode.file_opened) {
        f_close(&gcode.file);
        gcode.file_opened = false;
        gcode.has_thumbnail = false;
    }
}

bool screen_print_preview_data_t::gcode_file_exists() {
    FILINFO finfo = { 0 };
    return f_stat(gcode_file_path, &finfo) == FR_OK;
}

//FIXME simple solution not to brake functionality before release
//rewrite later
static bool suppress_draw = false;

void screen_print_preview_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    // In case the file is no longer present, close this screen.
    // (Most likely because of usb flash drive disconnection).
    if (!gcode_file_exists()) {
        Screens::Access()->Close(); //if an dialog is openned, it will be closed first
        return;
    }

    if (!suppress_draw && fs_did_filament_runout()) {
        suppress_draw = true;
        Sound_Play(eSOUND_TYPE_SingleBeep);
        const PhaseResponses btns = { Response::Yes, Response::No, Response::Ignore, Response::_none };
        // this MakeRAM is safe - vars->media_LFN is statically allocated (even though it may not be obvious at the first look)
        switch (MsgBoxTitle(string_view_utf8::MakeRAM((const uint8_t *)gcode_file_name),
            _("Filament not detected. Load filament now? Select NO to cancel, or IGNORE to disable the filament sensor and continue."),
            btns, 0, GuiDefaults::RectScreenBodyNoFoot)) {
        case Response::Yes: //YES - load
            gui_dlg_load_forced();
            break;
        case Response::No: //NO - cancel
            Screens::Access()->Close();
            suppress_draw = false;
            return;
        case Response::Ignore: //IGNORE - disable
            fs_disable();
            break;
        default:
            break;
        }
        suppress_draw = false;
        //window_draw(id);
    }

    if (!suppress_draw && event == WINDOW_EVENT_LOOP && gcode.has_thumbnail &&
        // Draw the thumbnail
        redraw_thumbnail) {
        FILE f = { 0 };
        f_lseek(&gcode.file, 0);
        if (f_gcode_thumb_open(&f, &gcode.file) == 0) {
            display::DrawPng(
                point_ui16(PADDING, PADDING + TITLE_HEIGHT + PADDING), &f);
            f_gcode_thumb_close(&f);
        } else {
            DBG("print preview: f_gcode_thumb_open returned non-zero value");
        }
        redraw_thumbnail = false;
    }

    window_frame_t::windowEvent(sender, event, param);
}
