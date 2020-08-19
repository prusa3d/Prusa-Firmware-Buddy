//screen_home.cpp
#include "screen_home.hpp"
#include "ff.h"
#include "dbg.h"

#include "config.h"

#include "marlin_client.h"
#include "screen_print_preview.hpp"
#include "screen_filebrowser.hpp"
#include "print_utils.hpp"

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "screen_menus.hpp"

#include "i18n.h"

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
    : window_frame_t()
    , header(this)
    , footer(this)
    , logo(this, Rect16(41, 31, 158, 40), IDR_PNG_status_logo_prusa_prn)
    , w_buttons { { this, Rect16(), 0, []() { Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>); } },
        { this, Rect16(), 0, []() { Screens::Access()->Open(GetScreenMenuPreheat); } },
        { this, Rect16(), 0, []() { Screens::Access()->Open(GetScreenMenuFilament); } },
        { this, Rect16(), 0, []() { Screens::Access()->Open(GetScreenMenuCalibration); } },
        { this, Rect16(), 0, []() { Screens::Access()->Open(GetScreenMenuSettings); } },
        { this, Rect16(), 0, []() { Screens::Access()->Open(GetScreenMenuInfo); } } }
    , w_labels { { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no } }

{
    // Every 49days and some time in 5 seconds window, auto filebrowser open will not work.
    // Seconds (timestamp) from UNIX epocho will fix this
    time = HAL_GetTick();
    is_starting = (time < 5000) ? 1 : 0;

    header.SetIcon(IDR_PNG_status_icon_home);
    header.SetText(_("HOME"));

    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            const size_t i = row * 3 + col;
            w_buttons[i].rect = Rect16(8 + (15 + 64) * col, 88 + (14 + 64) * row, 64, 64);
            w_buttons[i].SetIdRes(icons[i]);

            w_labels[i].rect = Rect16(80 * col, 152 + (15 + 64) * row, 80, 14);
            w_labels[i].font = resource_font(IDR_FNT_SMALL);
            w_labels[i].SetAlignment(ALIGN_CENTER);
            w_labels[i].SetPadding({ 0, 0, 0, 0 });
            w_labels[i].SetText(_(labels[i]));
        }
    }

    if (!marlin_vars()->media_inserted)
        printBtnDis();
}

void screen_home_data_t::draw() {
    window_frame_t::draw();
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    display::DrawText(Rect16(180, 31, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
#endif //_DEBUG
}

void screen_home_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {

    if (is_starting) // first 1000ms (cca 50ms is event period) skip MediaInserted
    {
        uint32_t now = HAL_GetTick();
        if ((now - time) > 950) {
            is_starting = 0;
        }

        //header.EventClr();
        if (header.EventClr_MediaInserted())
            printBtnEna();
    }

    //todo i think this should be handled in print preview
    if (header.EventClr_MediaInserted()) {
        if (HAL_GetTick() > 5000) {
            // we are using marlin variables for filename and filepath buffers
            marlin_vars_t *vars = marlin_vars();
            //check if the variables filename and filepath are allocated
            if (vars->media_SFN_path != nullptr && vars->media_LFN != nullptr) {
                if (find_latest_gcode(
                        vars->media_SFN_path,
                        FILE_PATH_MAX_LEN,
                        vars->media_LFN,
                        FILE_NAME_MAX_LEN)) {
                    screen_print_preview_data_t::SetGcodeFilepath(vars->media_SFN_path);
                    screen_print_preview_data_t::SetGcodeFilename(vars->media_LFN);
                    Screens::Access()->Open(ScreenFactory::Screen<screen_print_preview_data_t>);
                }
            }
        }
        printBtnEna();
    }

    if (header.EventClr_MediaRemoved()) {
        printBtnDis();
    }

    window_frame_t::windowEvent(sender, event, param);
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
    w_buttons[0].Unshadow();
    w_buttons[0].Enable(); // can be focused
    w_buttons[0].Invalidate();
    w_labels[0].SetText(_(labels[labelPrintId]));
}

void screen_home_data_t::printBtnDis() {
    w_buttons[0].Shadow();
    w_buttons[0].Disable(); // cant't be focused
    w_buttons[0].Invalidate();
    w_labels[0].SetText(_(labels[labelNoUSBId]));

    // move to preheat when Print is focused
    if (w_buttons[0].IsFocused()) {
        w_buttons[1].SetFocus();
    }
}
