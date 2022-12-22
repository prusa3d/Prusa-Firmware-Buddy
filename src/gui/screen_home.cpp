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
#include "png_resources.hpp"

#include "RAII.hpp"
#include "lazyfilelist.h"
#include "i18n.h"
#include "netdev.h"

#include "screen_menu_settings.hpp"

#include <crash_dump/crash_dump_handlers.hpp>

// TODO remove netdev_is_enabled after it is defined
bool __attribute__((weak)) netdev_is_enabled(const uint32_t netdev_id) { return true; }

bool screen_home_data_t::ever_been_opened = false;
bool screen_home_data_t::try_esp_flash = true;

static constexpr const png::Resource *icons[] = {
    &png::print_58x58,
    &png::preheat_58x58,
    &png::spool_58x58,
    &png::calibrate_58x58,
    &png::settings_58x58,
    &png::info_58x58
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

bool screen_home_data_t::usbWasAlreadyInserted = false;
uint32_t screen_home_data_t::lastUploadCount = 0;

screen_home_data_t::screen_home_data_t()
    : AddSuperWindow<screen_t>()
    , usbInserted(marlin_vars()->media_inserted)
    , event_in_progress(false)
    , header(this)
    , footer(this)
    , logo(this, Rect16(41, 31, 158, 40), &png::prusa_mini_logo_153x40)
    , w_buttons { { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>); } },
        { this, Rect16(), nullptr, []() { marlin_gcode_printf("M1700"); } },
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(GetScreenMenuFilament); } },
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(GetScreenMenuCalibration); } },
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuSettings>); } },
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuInfo>); } } }
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
    WindowFileBrowser::SetRoot("/usb");

    header.SetIcon(&png::home_shape_16x16);
#ifndef _DEBUG
    header.SetText(_("HOME"));
#else
    static const uint8_t msgHomeDebugRolling[] = "HOME - DEBUG - what a beautiful rolling text";
    header.SetText(string_view_utf8::MakeCPUFLASH(msgHomeDebugRolling)); // intentionally not translated
#endif

    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            const size_t i = row * 3 + col;
            w_buttons[i].SetRect(Rect16(8 + (15 + 64) * col, 88 + (14 + 64) * row, 64, 64));
            w_buttons[i].SetRes(icons[i]);

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
    ever_been_opened = true;
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

void screen_home_data_t::handle_crash_dump() {
    crash_dump::BufferT dump_buffer;
    const auto &present_dumps { crash_dump::get_present_dumps(dump_buffer) };
    if (present_dumps.size() == 0) {
        return;
    }
    if (MsgBoxWarning(_("Crash detected. Save it to USB and send it to Prusa?"), Responses_YesNo)
        == Response::Yes) {
        auto do_stage = [&](string_view_utf8 msg, std::invocable<const crash_dump::DumpHandler *> auto fp) {
            MsgBoxIconned box(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, std::move(msg), is_multiline::yes, &png::info_58x58);
            box.Show();
            draw();
            for (const auto &dump_handler : present_dumps) {
                fp(dump_handler);
            }
            box.Hide();
        };

        do_stage(_("Saving to USB"), [](const crash_dump::DumpHandler *handler) { handler->usb_save(); });
        do_stage(_("Sending to Prusa"), [](const crash_dump::DumpHandler *handler) { handler->server_upload(); });
    }

    for (const auto &dump_handler : present_dumps) {
        dump_handler->remove();
    }
}

void screen_home_data_t::on_enter() {
    if (!first_event) {
        return;
    }
    first_event = false;

    handle_crash_dump();
}

void screen_home_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    // TODO: This easily freezes home screen when flash action fails to start.
    // There are several places in the code where executing a flash gcode can
    // result in no-op and home screen stays active with events disabled.
    if (event_in_progress)
        return;

    AutoRestore avoid_recursion(event_in_progress, true);

    on_enter();

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

    if (event == GUI_event_t::LOOP) {

#if HAS_SELFTEST
        if (!DialogHandler::Access().IsOpen()) {
            //esp update has bigger priority tha one click print
            const auto fw_state = esp_fw_state();
            const bool esp_need_flash = fw_state == EspFwState::WrongVersion || fw_state == EspFwState::NoFirmware;
            if (try_esp_flash && esp_need_flash && netdev_is_enabled(NETDEV_ESP_ID)) {
                try_esp_flash = false; // do esp flash only once (user can press abort)
                marlin_gcode("M997 S1 O");
                return;
            } else {
                // on esp update, can use one click print
                if (GuiMediaEventsHandler::ConsumeOneClickPrinting() || moreGcodesUploaded()) {

                    // we are using marlin variables for filename and filepath buffers
                    marlin_vars_t *vars = marlin_vars();
                    // check if the variables filename and filepath are allocated
                    if (vars->media_SFN_path != nullptr && vars->media_LFN != nullptr) {

                        // TODO this should be done in main thread before MARLIN_EVT_MediaInserted is generated
                        // if it is not the latest gcode might not be selected
                        if (find_latest_gcode(
                                vars->media_SFN_path,
                                FILE_PATH_BUFFER_LEN,
                                vars->media_LFN,
                                FILE_NAME_BUFFER_LEN)) {
                            print_begin(vars->media_SFN_path, false);
                        }
                    }
                }
            }
        }
#endif // HAS_SELFTEST
    }

    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

bool screen_home_data_t::find_latest_gcode(char *fpath, int fpath_len, char *fname, int fname_len) {
    FileSort::LessFE_t LessFE = &FileSort::LessByTimeFE;
    FileSort::MakeEntry_t MakeLastEntry = &FileSort::MakeLastEntryByTime;

    fname[0] = 0;
    strlcpy(fpath, "/usb", fpath_len);
    F_DIR_RAII_Iterator dir(fpath);
    fpath[4] = '/';

    if (dir.result == ResType::NOK) {
        return false;
    }

    // prepare the item at the zeroth position according to sort policy
    FileSort::Entry entry = MakeLastEntry(); // last entry is greater than any file

    while (dir.FindNext()) {
        // skip folders
        if ((dir.fno->d_type & DT_DIR) != 0) {
            continue;
        }

        if (LessFE(dir.fno, entry)) {
            entry.CopyFrom(dir.fno);

            strlcpy(fpath + 5, dir.fno->d_name, fpath_len - 5);
            strlcpy(fname, entry.lfn, fname_len);
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

bool screen_home_data_t::moreGcodesUploaded() {
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
