/*
 * screen_prusa.c
 *
 *  Created on: 16. 7. 2019
 *      Author: mcbig
 */

#include "ff.h"
#include "dbg.h"
#include "gui.h"
#include "config.h"
#include "window_header.h"
#include "status_footer.h"
#include "marlin_client.h"
#include "screen_print_preview.h"
#include "screen_printing.h"
#include "print_utils.h"

#include "../Marlin/src/sd/cardreader.h"

extern screen_t *pscreen_filebrowser;
extern screen_t *pscreen_menu_preheat;
extern uint8_t menu_preheat_type;
extern screen_t *pscreen_menu_filament;
extern screen_t *pscreen_menu_calibration;
extern screen_t *pscreen_menu_settings;
extern screen_t *pscreen_menu_info;

#define BUTTON_PRINT       0
#define BUTTON_PREHEAT     1
#define BUTTON_FILAMENT    2
#define BUTTON_CALIBRATION 3
#define BUTTON_SETTINGS    4
#define BUTTON_INFO        5

const uint16_t icons[6] = {
    IDR_PNG_menu_icon_print,
    IDR_PNG_menu_icon_preheat,
    IDR_PNG_menu_icon_spool,
    IDR_PNG_menu_icon_calibration,
    IDR_PNG_menu_icon_settings,
    IDR_PNG_menu_icon_info
};

const char *labels[7] = {
    "Print",
    "Preheat",
    "Filament",
    "Calibration",
    "Settings",
    "Info",
    "No USB" // label variant for first button
};

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    window_frame_t root;

    window_header_t header;
    window_icon_t logo;

    window_icon_t w_buttons[6];
    window_text_t w_labels[6];

    status_footer_t footer;

    uint8_t is_starting;
    uint32_t time;
    uint8_t logo_invalid;
} screen_home_data_t;

#pragma pack(pop)

#define pw ((screen_home_data_t *)screen->pdata)

static bool find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len);
void screen_home_disable_print_button(screen_t *screen);

void screen_home_init(screen_t *screen) {
    // Every 49days and some time in 5 seconds window, auto filebrowser open did not work.
    // Seconds (timestamp) from UNIX epocho will fix this
    pw->time = HAL_GetTick();
    pw->is_starting = (pw->time < 5000) ? 1 : 0;

    int16_t id;

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0), &(pw->root));

    id = window_create_ptr(WINDOW_CLS_HEADER, root,
        rect_ui16(0, 0, 240, 31), &(pw->header));
    p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_home);
    p_window_header_set_text(&(pw->header), "HOME");

    id = window_create_ptr(WINDOW_CLS_ICON, root,
        rect_ui16(41, 31, 158, 40), &(pw->logo));
    window_set_icon_id(id, IDR_PNG_status_logo_prusa_prn);

    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            id = window_create_ptr(
                WINDOW_CLS_ICON, root,
                rect_ui16(8 + (15 + 64) * col, 88 + (14 + 64) * row, 64, 64),
                &(pw->w_buttons[row * 3 + col]));
            window_set_color_back(id, COLOR_GRAY);
            window_set_icon_id(id, icons[row * 3 + col]);
            window_set_tag(id, row * 3 + col + 1);
            window_enable(id);

            /*
				w_buttons[row*3+col] = window_icon_create(
						win,
						rect_ui16(8+(16+64)*col, 98+(14+64)*row, 64, 64),
						IDR_PNG_menu_icon_square,
						COLOR_BLACK);
			 */
            id = window_create_ptr(
                WINDOW_CLS_TEXT, root,
                rect_ui16(80 * col, 152 + (15 + 64) * row, 80, 14),
                &(pw->w_labels[row * 3 + col]));
            pw->w_labels[row * 3 + col].font = resource_font(IDR_FNT_SMALL);
            window_set_alignment(id, ALIGN_CENTER);
            window_set_padding(id, padding_ui8(0, 0, 0, 0));
            window_set_text(id, labels[row * 3 + col]);
        }
    }

    if (!IS_SD_INSERTED())
        screen_home_disable_print_button(screen);

    status_footer_init(&(pw->footer), root);
}

void screen_home_done(screen_t *screen) {
    window_destroy(pw->root.win.id);
}

void screen_home_draw(screen_t *screen) {
    if (pw->logo.win.f_invalid)
        pw->logo_invalid = 1;
}

static void on_print_preview_action(print_preview_action_t action) {
    if (action == PRINT_PREVIEW_ACTION_BACK) {
        screen_close(); // close the print preview
    } else if (action == PRINT_PREVIEW_ACTION_PRINT) {
        screen_close(); // close the print preview
        print_begin(screen_print_preview_get_gcode_filepath());
        screen_open(pscreen_printing->id);
    }
}

int screen_home_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (status_footer_event(&(pw->footer), window, event, param)) {
        return 1;
    }
    if ((event == WINDOW_EVENT_LOOP) && pw->logo_invalid) {
#ifdef _DEBUG
        display->draw_text(rect_ui16(180, 31, 60, 13), "DEBUG", resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
#endif //_DEBUG
        pw->logo_invalid = 0;
    }

    if (pw->is_starting) // first 1000ms (cca 50ms is event period) skip MediaInserted
    {
        uint32_t now = HAL_GetTick();
        if ((now - pw->time) > 950) {
            pw->is_starting = 0;
        }
        p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaInserted);
        p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaRemoved);
        p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaError);
    }

    if (p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaInserted) &&

        (HAL_GetTick() > 5000)) {
        // FIXME: currently, we are using the screen_printing_file_{name,path} buffers
        // ... and we should not be
        if (find_latest_gcode(
                screen_printing_file_path,
                sizeof(screen_printing_file_path),
                screen_printing_file_name,
                sizeof(screen_printing_file_name))) {
            screen_print_preview_set_gcode_filepath(screen_printing_file_path);
            screen_print_preview_set_gcode_filename(screen_printing_file_name);
            screen_print_preview_set_on_action(on_print_preview_action);
            screen_open(pscreen_print_preview->id);
        }
        return 1;
    }

    if (p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaRemoved)) {
        screen_home_disable_print_button(screen);
    }

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    switch ((int)param) {
    case BUTTON_PRINT + 1:
        screen_open(pscreen_filebrowser->id);
        return 1;
        break;
    case BUTTON_PREHEAT + 1:
        menu_preheat_type = 0;
        screen_open(pscreen_menu_preheat->id);
        return 1;
    case BUTTON_FILAMENT + 1:
        screen_open(pscreen_menu_filament->id);
        return 1;
    case BUTTON_CALIBRATION + 1:
        screen_open(pscreen_menu_calibration->id);
        return 1;
    case BUTTON_SETTINGS + 1:
        screen_open(pscreen_menu_settings->id);
        return 1;
    case BUTTON_INFO + 1:
        screen_open(pscreen_menu_info->id);
        return 1;
    }
    return 0;
}

static bool find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len) {
    DIR dir = { 0 };

    FRESULT result = f_opendir(&dir, "/");
    if (result != FR_OK) {
        return false;
    }

    fname[0] = 0;
    WORD latest_fdate = 0;
    WORD latest_ftime = 0;
    FILINFO current_finfo = { 0 };

    result = f_findfirst(&dir, &current_finfo, "", "*.gcode");
    while (result == FR_OK && current_finfo.fname[0]) {
        bool skip = current_finfo.fattrib & AM_SYS
            || current_finfo.fattrib & AM_HID;
        bool is_newer = latest_fdate != current_finfo.fdate
            ? latest_fdate < current_finfo.fdate
            : latest_ftime < current_finfo.ftime;

        if ((fname[0] == 0 || is_newer) && !skip) {
            const char *short_name = current_finfo.altname[0] ? current_finfo.altname : current_finfo.fname;
            snprintf(fpath, fpath_len, "/%s", short_name);
            snprintf(fname, fname_len, "%s", current_finfo.fname);
            latest_fdate = current_finfo.fdate;
            latest_ftime = current_finfo.ftime;
        }

        result = f_findnext(&dir, &current_finfo);
    }

    f_closedir(&dir);
    return result == FR_OK && fname[0] != 0 ? true : false;
}

void screen_home_disable_print_button(screen_t *screen) {
    pw->w_buttons[0].win.f_disabled = 1;
    pw->w_buttons[0].win.f_enabled = 0; // cant't be focused
    window_set_text(pw->w_labels[0].win.id, labels[6]);

    // move to preheat when Print is focused
    if (window_is_focused(pw->w_buttons[0].win.id)) {
        window_set_focus(pw->w_buttons[1].win.id);
    }
}

screen_t screen_home = {
    0, // id - will be generated
    0, // flags
    screen_home_init,
    screen_home_done,
    screen_home_draw,
    screen_home_event,
    sizeof(screen_home_data_t), //data_size
    0,                          //pdata
};

const screen_t *pscreen_home = &screen_home;
