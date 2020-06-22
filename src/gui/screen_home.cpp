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
#include "print_utils.h"

#include "screens.h"

#include "../lang/i18n.h"

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
    N_("Print"),
    N_("Preheat"),
    N_("Filament"),
    N_("Calibration"),
    N_("Settings"),
    N_("Info"),
    N_("No USB") // label variant for first button
};

struct screen_home_data_t {
    window_frame_t root;

    window_header_t header;
    window_icon_t logo;

    window_icon_t w_buttons[6];
    window_text_t w_labels[6];

    status_footer_t footer;

    uint8_t is_starting;
    uint32_t time;
    uint8_t logo_invalid;
};

#define pw ((screen_home_data_t *)screen->pdata)

static bool find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len);
void screen_home_disable_print_button(screen_t *screen, int disable);

void screen_home_init(screen_t *screen) {
    // Every 49days and some time in 5 seconds window, auto filebrowser open did not work.
    // Seconds (timestamp) from UNIX epocho will fix this
    pw->time = HAL_GetTick();
    pw->is_starting = (pw->time < 5000) ? 1 : 0;

    int16_t id;

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0), &(pw->root));

    id = window_create_ptr(WINDOW_CLS_HEADER, root, gui_defaults.header_sz, &(pw->header));
    p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_home);
    p_window_header_set_text(&(pw->header), _("HOME"));

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

    if (!marlin_vars()->media_inserted)
        screen_home_disable_print_button(screen, 1);

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
        screen_open(get_scr_printing()->id);
    }
}

int screen_home_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (status_footer_event(&(pw->footer), window, event, param)) {
        return 1;
    }
    if ((event == WINDOW_EVENT_LOOP) && pw->logo_invalid) {
#ifdef _DEBUG
        display::DrawText(rect_ui16(180, 31, 60, 13), "DEBUG", resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
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
        // we are using marlin variables for filename and filepath buffers
        marlin_vars_t *vars = marlin_vars();
        //check if the variables filename and filepath allocated
        if (vars->media_LFN && vars->media_LFN) {
            if (find_latest_gcode(
                    vars->media_SFN_path,
                    FILE_PATH_MAX_LEN,
                    vars->media_LFN,
                    FILE_NAME_MAX_LEN)) {
                screen_print_preview_set_gcode_filepath(vars->media_SFN_path);
                screen_print_preview_set_gcode_filename(vars->media_LFN);
                screen_print_preview_set_on_action(on_print_preview_action);
                screen_open(get_scr_print_preview()->id);
            }
            screen_home_disable_print_button(screen, 0);
        }
        return 1;
    }

    if (p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaRemoved)) {
        screen_home_disable_print_button(screen, 1);
    }

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    switch ((int)param) {
    case BUTTON_PRINT + 1:
        screen_open(get_scr_filebrowser()->id);
        return 1;
        break;
    case BUTTON_PREHEAT + 1:
        screen_open(get_scr_menu_preheat()->id);
        return 1;
    case BUTTON_FILAMENT + 1:
        screen_open(get_scr_menu_filament()->id);
        return 1;
    case BUTTON_CALIBRATION + 1:
        screen_open(get_scr_menu_calibration()->id);
        return 1;
    case BUTTON_SETTINGS + 1:
        screen_open(get_scr_menu_settings()->id);
        return 1;
    case BUTTON_INFO + 1:
        screen_open(get_scr_menu_info()->id);
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
            fpath[0] = '/';
            strlcpy(fpath + 1, short_name, fpath_len - 1);
            strlcpy(fname, current_finfo.fname, fname_len);
            latest_fdate = current_finfo.fdate;
            latest_ftime = current_finfo.ftime;
        }

        result = f_findnext(&dir, &current_finfo);
    }

    f_closedir(&dir);
    return result == FR_OK && fname[0] != 0 ? true : false;
}

void screen_home_disable_print_button(screen_t *screen, int disable) {
    pw->w_buttons[0].win.f_disabled = disable;
    pw->w_buttons[0].win.f_enabled = !disable; // cant't be focused
    pw->w_buttons[0].win.f_invalid = 1;
    window_set_text(pw->w_labels[0].win.id, labels[(disable ? 6 : 0)]);

    // move to preheat when Print is focused
    if (window_is_focused(pw->w_buttons[0].win.id) && disable) {
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

screen_t *const get_scr_home() { return &screen_home; }
