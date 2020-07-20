//screen_home.cpp
#include "screen_home.hpp"
#include "ff.h"
#include "dbg.h"

#include "config.h"

#include "marlin_client.h"
#include "screen_print_preview.hpp"
#include "print_utils.h"

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "screen_menus.hpp"

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

constexpr size_t labelPrintId = 0;
constexpr size_t labelNoUSBId = 6;

const char *labels[7] = {
    N_("Print"),
    N_("Preheat"),
    N_("Filament"),
    N_("Calibration"),
    N_("Settings"),
    N_("Info"),
    N_("No USB") // label variant for first button
};
static bool find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len);

screen_home_data_t::screen_home_data_t()
    : window_frame_t(&header)
    , header(this)
    , footer(this)
    , logo(this, rect_ui16(41, 31, 158, 40), IDR_PNG_status_logo_prusa_prn)
    , w_buttons { { this, { 0 }, 0 },
        { this, { 0 }, 0 },
        { this, { 0 }, 0 },
        { this, { 0 }, 0 },
        { this, { 0 }, 0 },
        { this, { 0 }, 0 } }
    , w_labels { { this, { 0 } },
        { this, { 0 } },
        { this, { 0 } },
        { this, { 0 } },
        { this, { 0 } },
        { this, { 0 } } }

{
    // Every 49days and some time in 5 seconds window, auto filebrowser open will not work.
    // Seconds (timestamp) from UNIX epocho will fix this
    time = HAL_GetTick();
    is_starting = (time < 5000) ? 1 : 0;

    header.SetIcon(IDR_PNG_status_icon_home);
    header.SetText(_("HOME"));

    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            //w_buttons[row * 3 + col].SetBackColor(COLOR_GRAY); //this did not work before, do we want it?
            w_buttons[row * 3 + col].rect = rect_ui16(8 + (15 + 64) * col, 88 + (14 + 64) * row, 64, 64);
            w_buttons[row * 3 + col].SetIdRes(icons[row * 3 + col]);
            w_buttons[row * 3 + col].SetTag(row * 3 + col + 1);
            w_buttons[row * 3 + col].Enable();

            w_labels[row * 3 + col].rect = rect_ui16(80 * col, 152 + (15 + 64) * row, 80, 14);
            w_labels[row * 3 + col].font = resource_font(IDR_FNT_SMALL);
            w_labels[row * 3 + col].SetAlignment(ALIGN_CENTER);
            w_labels[row * 3 + col].SetPadding(padding_ui8(0, 0, 0, 0));
            w_labels[row * 3 + col].SetText(_(labels[row * 3 + col]));
        }
    }

    w_buttons[0].SetFocus();
    if (!marlin_vars()->media_inserted)
        printBtnDis();
}

void screen_home_data_t::draw() {
    window_frame_t::draw();
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    display::DrawText(rect_ui16(180, 31, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
#endif //_DEBUG
}

static void on_print_preview_action(print_preview_action_t action) {
    if (action == PRINT_PREVIEW_ACTION_BACK) {
        Screens::Access()->Close(); // close the print preview
    } else if (action == PRINT_PREVIEW_ACTION_PRINT) {
        Screens::Access()->Close(); // close the print preview
        print_begin(screen_print_preview_get_gcode_filepath());
        //screen_open(get_scr_printing()->id);
    }
}

int screen_home_data_t::event(window_t *sender, uint8_t event, void *param) {
    /* if (status_footer_event(&(footer), sender, event, param)) {
        return 1;
    }*/

    if (is_starting) // first 1000ms (cca 50ms is event period) skip MediaInserted
    {
        uint32_t now = HAL_GetTick();
        if ((now - time) > 950) {
            is_starting = 0;
        }
        header.EventClr();
    }

    /*if (p_window_header_event_clr(&(header), MARLIN_EVT_MediaInserted) && (HAL_GetTick() > 5000)) {
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
                //screen_open(get_scr_print_preview()->id);
            }
            printBtnEna();
        }
        return 1;
    }*/

    if (header.EventClr_MediaRemoved()) {
        printBtnDis();
    }
    //

    if (event != WINDOW_EVENT_CLICK) {
        return window_frame_t::event(sender, event, param);
    }

    switch ((int)param) {
    case BUTTON_PRINT + 1:
        //screen_open(get_scr_filebrowser()->id);
        return 1;
    case BUTTON_PREHEAT + 1:
        Screens::Access()->Open(GetScreenMenuPreheat);
        return 1;
    case BUTTON_FILAMENT + 1:
        //screen_open(get_scr_menu_filament()->id);
        return 1;
    case BUTTON_CALIBRATION + 1:
        //screen_open(get_scr_menu_calibration()->id);
        return 1;
    case BUTTON_SETTINGS + 1:
        Screens::Access()->Open(GetScreenMenuSettings);
        return 1;
    case BUTTON_INFO + 1:
        Screens::Access()->Open(GetScreenMenuInfo);
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

void screen_home_data_t::printBtnEna() {
    w_buttons[0].UnswapBW();
    w_buttons[0].Enable(); // can be focused
    w_buttons[0].Invalidate();
    w_labels[0].SetText(_(labels[labelPrintId]));
}

void screen_home_data_t::printBtnDis() {
    w_buttons[0].SwapBW();
    w_buttons[0].Disable(); // cant't be focused
    w_buttons[0].Invalidate();
    w_labels[0].SetText(_(labels[labelNoUSBId]));

    // move to preheat when Print is focused
    if (w_buttons[0].IsFocused()) {
        w_buttons[1].SetFocus();
    }
}
