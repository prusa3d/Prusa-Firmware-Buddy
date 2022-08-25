// screen_home.cpp
#include "screen_home.hpp"
#include "stdio.h"
#include "file_raii.hpp"

#include "config.h"

#include "marlin_client.h"
#include "screen_print_preview.hpp"
#include "screen_filebrowser.hpp"
#include "print_utils.hpp"
#include <wui_api.h>
#include <espif.h>

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "screen_menus.hpp"
#include "gui_media_events.hpp"
#include "window_dlg_load_unload.hpp"
#include "DialogMoveZ.hpp"
#include "DialogHandler.hpp"

#include "i18n.h"

bool screen_home_data_t::ever_been_openned = false;
bool screen_home_data_t::try_esp_flash = true;

const uint16_t icons[] = {
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

bool screen_home_data_t::usbWasAlreadyInserted = false;

screen_home_data_t::screen_home_data_t()
    : AddSuperWindow<screen_t>()
    , usbInserted(marlin_vars()->media_inserted)
    , esp_flash_being_openned(false)
    , header(this)
    , footer(this)
    , logo(this, Rect16(41, 31, 158, 40), IDR_PNG_prusa_printer_logo)
    , w_buttons { { this, Rect16(), 0, []() { Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>); } },
        { this, Rect16(), 0, []() { marlin_gcode_printf("M1700"); } },
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
    , gcode(GCodeInfo::getInstance())

{
    EnableLongHoldScreenAction();
    window_frame_t::ClrMenuTimeoutClose();
    window_frame_t::ClrOnSerialClose(); // don't close on Serial print
    screen_filebrowser_data_t::SetRoot("/usb");

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
            w_buttons[i].SetRect(Rect16(8 + (15 + 64) * col, 88 + (14 + 64) * row, 64, 64));
            w_buttons[i].SetIdRes(icons[i]);

            w_labels[i].SetRect(Rect16(80 * col, 154 + (15 + 64) * row, 80, 14));
            w_labels[i].font = resource_font(IDR_FNT_SMALL);
            w_labels[i].SetAlignment(Align_t::Center());
            w_labels[i].SetPadding({ 0, 0, 0, 0 });
            w_labels[i].SetText(_(labels[i]));
        }
    }

    if (!usbInserted) {
        printBtnDis();
    } else {
        usbWasAlreadyInserted = true;
    }
    ever_been_openned = true;
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
    if (esp_flash_being_openned)
        return;

    if (event == GUI_event_t::MEDIA) {
        switch (MediaState_t(int(param))) {
        case MediaState_t::inserted:
            if (!usbInserted) {
                usbInserted = true;
                printBtnEna();
                if (!usbWasAlreadyInserted) {
                    w_buttons[0].SetFocus(); // print button
                    usbWasAlreadyInserted = true;
                }
            }
            break;
        case MediaState_t::removed:
        case MediaState_t::error:
            if (usbInserted) {
                usbInserted = false;
                printBtnDis();
            }
            break;
        default:
            break;
        }
    }

    if (event == GUI_event_t::LOOP && !DialogHandler::Access().IsOpen()) {
        //esp update has bigger priority tha one click print
        const auto fw_state = esp_fw_state();
        if (try_esp_flash && (fw_state == EspFwState::WrongVersion || fw_state == EspFwState::NoFirmware)) {
            try_esp_flash = false;          // do esp flash only once (user can press abort)
            esp_flash_being_openned = true; // wait for process of gcode == open of flash screen
            marlin_gcode("M997 S1 O");
            return;
        } else {

            if (GuiMediaEventsHandler::ConsumeOneClickPrinting()) {

                // we are using marlin variables for filename and filepath buffers
                marlin_vars_t *vars = marlin_vars();
                // check if the variables filename and filepath are allocated
                if (vars->media_SFN_path != nullptr && vars->media_LFN != nullptr) {
                    if (find_latest_gcode(
                            vars->media_SFN_path,
                            FILE_PATH_BUFFER_LEN,
                            vars->media_LFN,
                            FILE_NAME_BUFFER_LEN)) {
                        gcode.SetGcodeFilepath(vars->media_SFN_path);
                        gcode.SetGcodeFilename(vars->media_LFN);
                        Screens::Access()->Open(ScreenFactory::Screen<screen_print_preview_data_t>);
                    }
                }
            } else if (MoreGcodesUploaded()) { // on esp update, can use one click print

                // latest gcode is stored in marlin vars
                // but this ones does not auto update, so we need to call refresh manually
                marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_FILENAME) | MARLIN_VAR_MSK(MARLIN_VAR_FILEPATH));
                // check if the variables filename and filepath are allocated
                if (vars->media_SFN_path != nullptr && vars->media_LFN != nullptr) {
                    gcode.SetGcodeFilepath(vars->media_SFN_path);
                    gcode.SetGcodeFilename(vars->media_LFN);
                    if (GCodeInfo::getInstance().initFile()) {
                        Screens::Access()->Open(ScreenFactory::Screen<screen_print_preview_data_t>);
                    }
                }
            }
        }
    }
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

static bool find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len) {

    fname[0] = 0;
    strlcpy(fpath, "/usb", fpath_len);
    time_t latest_time = 0;
    F_DIR_RAII_Iterator dir(fpath);
    fpath[4] = '/';

    if (dir.result == ResType::NOK) {
        return false;
    }

    while (dir.FindNext()) {
        // skip folders
        if ((dir.fno->d_type & DT_DIR) != 0) {
            continue;
        }
        bool is_newer = latest_time < dir.fno->time;

        if (is_newer) {
            strlcpy(fpath + 5, dir.fno->d_name, fpath_len - 5);
            strlcpy(fname, dir.fno->lfn, fname_len);
        }
    }

    return fname[0] != 0;
}

void screen_home_data_t::printBtnEna() {
    w_buttons[0].Unshadow();
    w_buttons[0].Enable(); // can be focused
    w_buttons[0].Invalidate();
    w_labels[0].SetText(_(labels[labelPrintId]));
}

void screen_home_data_t::printBtnDis() {
    // move to preheat when Print is focused
    if (w_buttons[0].IsFocused()) {
        w_buttons[1].SetFocus();
    }

    w_buttons[0].Shadow();
    w_buttons[0].Disable(); // cant't be focused
    w_buttons[0].Invalidate();
    w_labels[0].SetText(_(labels[labelNoUSBId]));
}

bool screen_home_data_t::MoreGcodesUploaded() {
    static uint32_t lastUploadCount = 0;
    const uint32_t total = wui_gcodes_uploaded();
    const bool result = total != lastUploadCount;
    lastUploadCount = total;
    return result;
}

void screen_home_data_t::InitState(screen_init_variant var) {
    if (!var.GetPosition())
        return;

    size_t pos = *(var.GetPosition());
    if (pos >= button_count)
        return;

    w_buttons[pos].SetFocus();
}

screen_init_variant screen_home_data_t::GetCurrentState() const {
    screen_init_variant ret;
    for (size_t i = 0; i < button_count; ++i) {
        if (w_buttons[i].IsFocused()) {
            ret.SetPosition(i);
            return ret;
        }
    }
    return ret;
}
