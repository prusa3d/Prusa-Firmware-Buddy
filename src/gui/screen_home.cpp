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
#include "gui_media_events.hpp"

#include "i18n.h"

const uint16_t icons[6] = {
    IDR_PNG_print_58px,
    IDR_PNG_preheat_58px,
    IDR_PNG_spool_58px,
    IDR_PNG_calibrate_58px,
    IDR_PNG_settings_58px,
    IDR_PNG_info_58px
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
    : AddSuperWindow<window_frame_t>()
    , usbInserted(marlin_vars()->media_inserted)
    , header(this)
    , footer(this)
    , logo(this, Rect16(41, 31, 158, 40), IDR_PNG_prusa_printer_logo)
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
    window_frame_t::ClrMenuTimeoutClose();
    window_frame_t::ClrOnSerialClose(); // don't close on Serial print

    header.SetIcon(IDR_PNG_home_shape_16px);
#ifndef _DEBUG
    header.SetText(_("HOME"));
#else
    static const uint8_t msgHomeDebugRolling[] = "HOME - DEBUG - what a beautifull rolling text";
    header.SetText(string_view_utf8::MakeCPUFLASH(msgHomeDebugRolling)); // intentionally not translated
#endif

    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            const size_t i = row * 3 + col;
            w_buttons[i].rect = Rect16(8 + (15 + 64) * col, 88 + (14 + 64) * row, 64, 64);
            w_buttons[i].SetIdRes(icons[i]);

            w_labels[i].rect = Rect16(80 * col, 154 + (15 + 64) * row, 80, 14);
            w_labels[i].font = resource_font(IDR_FNT_SMALL);
            w_labels[i].SetAlignment(ALIGN_CENTER);
            w_labels[i].SetPadding({ 0, 0, 0, 0 });
            w_labels[i].SetText(_(labels[i]));
        }
    }

    if (!usbInserted) {
        printBtnDis();
    }
}

screen_home_data_t::~screen_home_data_t() {
    GuiMediaEventsHandler::ConsumeOneClickPrinting();
}

void screen_home_data_t::draw() {
    super::draw();
#ifdef _DEBUG
    static const char dbg[] = "DEBUG";
    display::DrawText(Rect16(180, 31, 60, 13), string_view_utf8::MakeCPUFLASH((const uint8_t *)dbg), resource_font(IDR_FNT_SMALL), COLOR_BLACK, COLOR_RED);
#endif //_DEBUG
}

void screen_home_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    if (event == GUI_event_t::MEDIA) {
        switch (GuiMediaEventsHandler::state_t(int(param))) {
        case GuiMediaEventsHandler::state_t::inserted:
            if (!usbInserted) {
                usbInserted = true;
                printBtnEna();
            }
            break;
        case GuiMediaEventsHandler::state_t::removed:
        case GuiMediaEventsHandler::state_t::error:
            if (usbInserted) {
                usbInserted = false;
                printBtnDis();
            }
            break;
        default:
            break;
        }
    }

    if (event == GUI_event_t::LOOP && GuiMediaEventsHandler::ConsumeOneClickPrinting()) {

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

    SuperWindowEvent(sender, event, param);
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
