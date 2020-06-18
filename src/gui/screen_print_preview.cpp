#include "screen_print_preview.h"
#include "dbg.h"
#include "ff.h"
#include "gcode_file.h"
#include "gui.h"
#include "marlin_client.h"
#include "resource.h"
#include "window_dlg_load_unload.h"
#include "filament_sensor.h"
#include <stdarg.h>
#include <stdbool.h>
#include "screens.h"
#include "sound_C_wrapper.h"
#include "DialogHandler.hpp"

#define DBG _dbg0

typedef struct {
    window_text_t title;
    window_text_t value;
    char value_buffer[32];
} description_line_t;

typedef struct {
    window_frame_t frame;
    window_text_t title_text;
    description_line_t description_lines[4];
    window_icon_t print_button;
    window_text_t print_label;
    window_icon_t back_button;
    window_text_t back_label;
    FIL gcode_file;
    bool gcode_file_opened;
    bool gcode_has_thumbnail;
    char gcode_printing_time[16];
    char gcode_filament_type[8];
    unsigned gcode_filament_used_g;
    unsigned gcode_filament_used_mm;
    bool redraw_thumbnail;
} screen_print_preview_data_t;

#define PADDING          10
#define SCREEN_WIDTH     240 //FIXME should be in display.h
#define SCREEN_HEIGHT    320 //FIXME should be in display.h
#define THUMBNAIL_HEIGHT 124
#define TITLE_HEIGHT     24
#define LINE_HEIGHT      15
#define LINE_SPACING     5

#define BACK_BUTTON_ID  0x11
#define PRINT_BUTTON_ID 0x12

static const char *gcode_file_name = NULL;
static const char *gcode_file_path = NULL;
static print_preview_action_handler_t action_handler = NULL;
static void screen_print_preview_init(screen_t *screen);
static void screen_print_preview_done(screen_t *screen);
static void screen_print_preview_draw(screen_t *screen);
static int screen_print_preview_event(screen_t *screen, window_t *window,
    uint8_t event, void *param);

screen_t screen_print_preview = {
    0, // screen identifier
    0, // flags
    screen_print_preview_init,
    screen_print_preview_done,
    screen_print_preview_draw,
    screen_print_preview_event,
    sizeof(screen_print_preview_data_t), // dynamic data size
    NULL                                 // dynamic data pointer
};

const uint16_t menu_icons[2] = {
    IDR_PNG_menu_icon_print,
    IDR_PNG_menu_icon_stop,
};

screen_t *const get_scr_print_preview() { return &screen_print_preview; }

#define pd ((screen_print_preview_data_t *)screen->pdata)

void screen_print_preview_set_gcode_filepath(const char *fpath) {
    gcode_file_path = fpath;
}

const char *screen_print_preview_get_gcode_filepath() {
    return gcode_file_path;
}

void screen_print_preview_set_gcode_filename(const char *fname) {
    gcode_file_name = fname;
}

void screen_print_preview_set_on_action(
    print_preview_action_handler_t handler) {
    action_handler = handler;
}

static void initialize_description_line(screen_t *screen, int idx, int y_pos,
    const char *title,
    const char *value_fmt, ...) {
    description_line_t *line = &pd->description_lines[idx];
    int window_id = pd->frame.win.id;

    int title_width = strlen(title) * resource_font(IDR_FNT_SMALL)->w;
    int title_id = window_create_ptr(
        WINDOW_CLS_TEXT, window_id,
        rect_ui16(PADDING, y_pos, title_width, LINE_HEIGHT), &line->title);
    window_set_text(title_id, title);
    window_set_alignment(title_id, ALIGN_LEFT_BOTTOM);
    window_set_padding(title_id, padding_ui8(0, 0, 0, 0));
    line->title.font = resource_font(IDR_FNT_SMALL);

    int value_width = SCREEN_WIDTH - PADDING * 2 - title_width - 1;
    int value_id = window_create_ptr(WINDOW_CLS_TEXT, window_id,
        rect_ui16(SCREEN_WIDTH - PADDING - value_width, y_pos,
            value_width, LINE_HEIGHT),
        &line->value);
    va_list args;
    va_start(args, value_fmt);
    vsnprintf(line->value_buffer, sizeof(line->value_buffer), value_fmt, args);
    va_end(args);
    window_set_text(value_id, line->value_buffer);
    window_set_alignment(value_id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(value_id, padding_ui8(0, 0, 0, 0));
    line->value.font = resource_font(IDR_FNT_SMALL);
}

static void initialize_description_lines(screen_t *screen, int y) {
    int line_idx = 0;

    // print time
    if (pd->gcode_printing_time[0]) {
        initialize_description_line(screen, line_idx++, y, "Print Time", "%s",
            pd->gcode_printing_time);
    } else {
        initialize_description_line(screen, line_idx++, y, "Print Time",
            "unknown");
    }
    y += LINE_HEIGHT + LINE_SPACING;

    if (pd->gcode_has_thumbnail) {
        // material
        if (pd->gcode_filament_type[0] && pd->gcode_filament_used_mm && pd->gcode_filament_used_g) {
            initialize_description_line(
                screen, line_idx++, y, "Material", "%s/%u g/%0.2f m",
                pd->gcode_filament_type, pd->gcode_filament_used_g,
                (double)((float)pd->gcode_filament_used_mm / 1000.0F));
            y += LINE_HEIGHT + LINE_SPACING;
        }
    } else {
        // material
        if (pd->gcode_filament_type[0]) {
            initialize_description_line(screen, line_idx++, y, "Material", "%s",
                pd->gcode_filament_type);
            y += LINE_HEIGHT + LINE_SPACING;
        }
        // used filament
        if (pd->gcode_filament_used_mm && pd->gcode_filament_used_g) {
            initialize_description_line(
                screen, line_idx++, y, "Used Filament", "%.2f m",
                (double)((float)pd->gcode_filament_used_mm / 1000.0F));
            y += LINE_HEIGHT + LINE_SPACING;

            initialize_description_line(screen, line_idx++, y, "", "%.0f g",
                (double)pd->gcode_filament_used_g);
            y += LINE_HEIGHT + LINE_SPACING;
        }
    }
}

static void initialize_gcode_file(screen_t *screen) {
    memset(&pd->gcode_file, 1, sizeof(FIL));
    pd->gcode_file_opened = false;
    pd->gcode_has_thumbnail = false;

    // try to open the file first
    if (!gcode_file_path || f_open(&pd->gcode_file, gcode_file_path, FA_READ) != FR_OK) {
        return;
    }
    pd->gcode_file_opened = true;

    // thumbnail presence check
    {
        FILE f = { 0 };
        if (f_gcode_thumb_open(&f, &pd->gcode_file) == 0) {
            char buffer;
            pd->gcode_has_thumbnail = fread((void *)&buffer, 1, 1, &f) > 0;
            f_gcode_thumb_close(&f);
        }
    }

    // find printing time and filament information
    pd->gcode_printing_time[0] = 0;
    pd->gcode_filament_type[0] = 0;
    pd->gcode_filament_used_mm = 0;
    pd->gcode_filament_used_g = 0;
    const unsigned search_last_x_bytes = 10000;
    FSIZE_t filesize = f_size(&pd->gcode_file);
    f_lseek(&pd->gcode_file, filesize > search_last_x_bytes ? filesize - search_last_x_bytes : 0);
    char name_buffer[64];
    char value_buffer[32];
    while (f_gcode_get_next_comment_assignment(
        &pd->gcode_file, name_buffer, sizeof(name_buffer), value_buffer,
        sizeof(value_buffer))) {

#define name_equals(str) (!strncmp(name_buffer, str, sizeof(name_buffer)))

        if (name_equals("estimated printing time (normal mode)")) {
            snprintf(pd->gcode_printing_time, sizeof(pd->gcode_printing_time),
                "%s", value_buffer);
        } else if (name_equals("filament_type")) {
            snprintf(pd->gcode_filament_type, sizeof(pd->gcode_filament_type),
                "%s", value_buffer);
        } else if (name_equals("filament used [mm]")) {
            sscanf(value_buffer, "%u", &pd->gcode_filament_used_mm);
        } else if (name_equals("filament used [g]")) {
            sscanf(value_buffer, "%u", &pd->gcode_filament_used_g);
        }
    }
}

static void screen_print_preview_init(screen_t *screen) {
    marlin_set_print_speed(100);
    initialize_gcode_file(screen);

    int window_id = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0), &pd->frame);
    window_enable(window_id);
    int y = PADDING;

    // Title
    int title_text_id = window_create_ptr(
        WINDOW_CLS_TEXT, window_id,
        rect_ui16(PADDING, y, SCREEN_WIDTH - 2 * PADDING, TITLE_HEIGHT),
        &pd->title_text);
    pd->title_text.font = resource_font(IDR_FNT_BIG);
    window_set_text(title_text_id, gcode_file_name);
    y += TITLE_HEIGHT + PADDING;

    // Thumbnail
    if (pd->gcode_has_thumbnail) {
        y += THUMBNAIL_HEIGHT + PADDING;
        // Drawing is done in screen_print_preview_event func
        pd->redraw_thumbnail = true;
    }

    // Description lines
    initialize_description_lines(screen, y);

    // Print and Back buttons
    y = SCREEN_HEIGHT - PADDING - LINE_HEIGHT - 64;
    int print_button_id = window_create_ptr(WINDOW_CLS_ICON, window_id,
        rect_ui16(PADDING, y, 64, 64), &pd->print_button);
    window_set_color_back(print_button_id, COLOR_GRAY);
    window_set_icon_id(print_button_id, IDR_PNG_menu_icon_print);
    window_set_tag(print_button_id, PRINT_BUTTON_ID);
    window_enable(print_button_id);
    int back_button_id = window_create_ptr(
        WINDOW_CLS_ICON, window_id,
        rect_ui16(SCREEN_WIDTH - PADDING - 64, y, 64, 64), &pd->back_button);
    window_set_color_back(back_button_id, COLOR_GRAY);
    window_set_icon_id(back_button_id, IDR_PNG_menu_icon_back);
    window_set_tag(back_button_id, BACK_BUTTON_ID);
    window_enable(back_button_id);

    // Print and Back labels
    y += 64;
    int print_label_id = window_create_ptr(
        WINDOW_CLS_TEXT, window_id, rect_ui16(PADDING, y, 64, LINE_HEIGHT),
        &pd->print_label);
    window_set_text(print_label_id, "Print");
    window_set_alignment(print_label_id, ALIGN_CENTER);
    pd->print_label.font = resource_font(IDR_FNT_SMALL);
    int back_label_id = window_create_ptr(
        WINDOW_CLS_TEXT, window_id,
        rect_ui16(SCREEN_WIDTH - PADDING - 64, y, 64, LINE_HEIGHT),
        &pd->back_label);
    window_set_text(back_label_id, "Back");
    window_set_alignment(back_label_id, ALIGN_CENTER);
    pd->back_label.font = resource_font(IDR_FNT_SMALL);
}

static void screen_print_preview_done(screen_t *screen) {
    if (pd->gcode_file_opened) {
        f_close(&pd->gcode_file);
        pd->gcode_file_opened = false;
        pd->gcode_has_thumbnail = false;
    }
    window_destroy(pd->frame.win.id);
}

static void screen_print_preview_draw(screen_t *screen) {
}

static bool gcode_file_exists(screen_t *screen) {
    FILINFO finfo = { 0 };
    return f_stat(gcode_file_path, &finfo) == FR_OK;
}

//FIXME simple solution not to brake functionality before release
//rewrite later
static bool suppress_draw = false;

static int screen_print_preview_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {
    // In case the file is no longer present, close this screen.
    // (Most likely because of usb flash drive disconnection).
    if (!gcode_file_exists(screen)) {
        screen_close();
        return 0;
    }

    if (!suppress_draw && fs_did_filament_runout()) {
        suppress_draw = true;
        Sound_Play(eSOUND_TYPE_StandardAlert);
        const char *btns[3] = { "YES", "NO", "IGNORE" };
        switch (gui_msgbox_ex(0,
            "Filament not detected. Load filament now? Select NO to cancel, or IGNORE to disable the filament sensor and continue.",
            MSGBOX_BTN_CUSTOM3,
            gui_defaults.scr_body_no_foot_sz,
            0, btns)) {
        case MSGBOX_RES_CUSTOM0: //YES - load
            gui_dlg_load_forced();
            //opens load dialog if it is not already openned
            DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Load));
            break;
        case MSGBOX_RES_CUSTOM1: //NO - cancel
            if (action_handler) {
                action_handler(PRINT_PREVIEW_ACTION_BACK);
            }
            suppress_draw = false;
            return 1;
        case MSGBOX_RES_CUSTOM2: //IGNORE - disable
            fs_disable();
            break;
        }
        suppress_draw = false;
        window_draw(pd->frame.win.id);
    }

    if (!suppress_draw && event == WINDOW_EVENT_LOOP && pd->gcode_has_thumbnail &&
        // Draw the thumbnail
        pd->redraw_thumbnail) {
        FILE f = { 0 };
        f_lseek(&pd->gcode_file, 0);
        if (f_gcode_thumb_open(&f, &pd->gcode_file) == 0) {
            display::DrawPng(
                point_ui16(PADDING, PADDING + TITLE_HEIGHT + PADDING), &f);
            f_gcode_thumb_close(&f);
        } else {
            DBG("print preview: f_gcode_thumb_open returned non-zero value");
        }
        pd->redraw_thumbnail = false;
    } else if (event == WINDOW_EVENT_CLICK && (intptr_t)param == PRINT_BUTTON_ID) {
        if (action_handler) {
            action_handler(PRINT_PREVIEW_ACTION_PRINT);
            return 1;
        }
    } else if (event == WINDOW_EVENT_CLICK && (intptr_t)param == BACK_BUTTON_ID) {
        if (action_handler) {
            action_handler(PRINT_PREVIEW_ACTION_BACK);
            return 1;
        }
    }

    return 0;
}
const char *screen_print_preview_get_gcode_filepath();
