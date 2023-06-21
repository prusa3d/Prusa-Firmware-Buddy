// screen_home.cpp
#include "screen_home.hpp"
#include "stdio.h"
#include "file_raii.hpp"

#include "config.h"

#include "marlin_client.hpp"
#include "screen_filebrowser.hpp"
#include "print_utils.hpp"
#include "gui_fsensor_api.hpp"
#include <wui_api.h>
#include <espif.h>

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "gui_media_events.hpp"
#include "DialogMoveZ.hpp"
#include "DialogHandler.hpp"
#include "png_resources.hpp"

#include "screen_printing.hpp"
#include "filament_sensors_handler.hpp"

#include "RAII.hpp"
#include "lazyfilelist.h"
#include "i18n.h"
#include "i2c.h"
#include "netdev.h"

#include <option/has_control_menu.h>
#include <option/has_loadcell.h>
#include <option/developer_mode.h>
#include <option/development_items.h>
#include <device/peripherals.h>

#include "screen_menu_settings.hpp"
#include "screen_menu_calibration.hpp"
#include "screen_menu_filament.hpp"
#if HAS_CONTROL_MENU()
    #include "screen_menu_control.hpp"
#endif

#if HAS_MMU2
    #include "screen_menu_filament_mmu.hpp"
#endif

#include <crash_dump/crash_dump_handlers.hpp>
#include "box_unfinished_selftest.hpp"
#include <option/has_control_menu.h>

// TODO remove netdev_is_enabled after it is defined
bool __attribute__((weak)) netdev_is_enabled([[maybe_unused]] const uint32_t netdev_id) { return true; }

bool screen_home_data_t::ever_been_opened = false;
bool screen_home_data_t::try_esp_flash = true;
bool screen_home_data_t::touch_broken_during_run = false;

#ifdef USE_ST7789
    #define GEN_ICON_NAMES(ICON) \
        { png::ICON##_64x64, png::ICON##_64x64_focused, png::ICON##_64x64_disabled }
#endif // USE_ST7789
#ifdef USE_ILI9488
    #define GEN_ICON_NAMES(ICON) \
        { png::ICON##_80x80, png::ICON##_80x80_focused, png::ICON##_80x80_disabled }
#endif // USE_ILI9488

static constexpr const WindowMultiIconButton::Pngs icons[] = {
    GEN_ICON_NAMES(print),
    GEN_ICON_NAMES(preheat),
    GEN_ICON_NAMES(spool),
    GEN_ICON_NAMES(calibrate),
    GEN_ICON_NAMES(settings),
    GEN_ICON_NAMES(info),
    GEN_ICON_NAMES(spools)
};

constexpr size_t labelPrintId = 0;
constexpr size_t labelNoUSBId = 6;
constexpr size_t iconNonMMUId = 2;
constexpr size_t iconMMUId = 6;
constexpr size_t buttonFilamentIndex = 2;

#ifdef USE_ST7789
constexpr size_t buttonsXSpacing = 15;
constexpr size_t buttonTextWidth = 80;
constexpr size_t buttonTextHeight = 13; // font_7x13

constexpr size_t buttonTopOffset = 88;
constexpr size_t buttonTextTopOffset = 155;

constexpr Rect16 logoRect = Rect16(41, 31, 158, 40);
#endif // USE_ST7789

#ifdef USE_ILI9488
constexpr size_t buttonsXSpacing = 40;
constexpr size_t buttonTextWidth = 94;
constexpr size_t buttonTextHeight = 23;

constexpr size_t buttonTopOffset = 53;
constexpr size_t buttonTextTopOffset = buttonTopOffset + GuiDefaults::ButtonIconSize + 5;
#endif // USE_ILI9488

constexpr size_t buttonTextSpacing = GuiDefaults::ButtonIconSize + buttonsXSpacing - buttonTextWidth;
constexpr size_t buttonsLeftOffset = (GuiDefaults::ScreenWidth - 3 * GuiDefaults::ButtonIconSize - 2 * buttonsXSpacing) / 2;
constexpr size_t buttonsTextsLeftOffset = (GuiDefaults::ScreenWidth - 3 * buttonTextWidth - 2 * buttonTextSpacing) / 2;

static constexpr Rect16 buttonRect(size_t col, size_t row) {
    return Rect16(
        buttonsLeftOffset + (buttonsXSpacing + GuiDefaults::ButtonIconSize) * col,
        buttonTopOffset + (GuiDefaults::ButtonIconVerticalSpacing + GuiDefaults::ButtonIconSize) * row,
        GuiDefaults::ButtonIconSize,
        GuiDefaults::ButtonIconSize);
}

static constexpr Rect16 buttonTextRect(size_t col, size_t row) {
    return Rect16(
        buttonsTextsLeftOffset + (buttonsXSpacing + GuiDefaults::ButtonIconSize) * col,
        buttonTextTopOffset + (GuiDefaults::ButtonIconVerticalSpacing + GuiDefaults::ButtonIconSize) * row,
        buttonTextWidth,
        buttonTextHeight);
}

const char *labels[] = {
    N_("Print"),
    N_("Preheat"),
    N_("Filament"),
#if HAS_CONTROL_MENU()
    N_("Control"),
#else
    N_("Calibrate"),
#endif
    N_("Settings"),
    N_("Info"),
    N_("No USB") // label variant for first button
};

bool screen_home_data_t::usbWasAlreadyInserted = false;

static void FilamentBtn_cb() {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilament>);
}

static void FilamentBtnMMU_cb() {
#if HAS_MMU2
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilamentMMU>);
#else
    FilamentBtn_cb();
#endif
}

// clang-format off
screen_home_data_t::screen_home_data_t()
    : AddSuperWindow<screen_t>()
    , usbInserted(marlin_vars()->media_inserted)
    , mmu_state(MMU2::xState::Stopped)
    , event_in_progress(false)
    , header(this)
    , footer(this)
#ifdef USE_ST7789
    , logo(this, logoRect, &png::printer_logo)
#endif // USE_ST7789
    , w_buttons {
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>); } },
        { this, Rect16(), nullptr, []() { marlin_gcode_printf("M1700"); } },
        { this, Rect16(), nullptr, FilamentBtn_cb },
#if HAS_CONTROL_MENU()
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuControl>); } },
#else
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuCalibration>); } },
#endif
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuSettings>); } },
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuInfo>); }}
    },
    w_labels {
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no },
        { this, Rect16(), is_multiline::no }
    },
    gcode(GCodeInfo::getInstance()) {
    // clang-format on

    EnableLongHoldScreenAction();
    window_frame_t::ClrMenuTimeoutClose();
    window_frame_t::ClrOnSerialClose(); // don't close on Serial print
    WindowFileBrowser::SetRoot("/usb");

    header.SetIcon(&png::home_shape_16x16);
#if !defined(_DEBUG) && !DEVELOPER_MODE()
    // regular home screen
    header.SetText(_("HOME"));
#else
    // show the appropriate build header
    #if DEVELOPER_MODE() && defined(_DEBUG)
    static const uint8_t msgHome[] = "HOME - DEV - DEBUG";
    #elif DEVELOPER_MODE() && !defined(_DEBUG)
    static const uint8_t msgHome[] = "HOME - DEV";
    #else
    static const uint8_t msgHome[] = "HOME - DEBUG - what a beautiful rolling text";
    #endif
    header.SetText(string_view_utf8::MakeCPUFLASH(msgHome)); // intentionally not translated
#endif

    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            const size_t i = row * 3 + col;
            w_buttons[i].SetRect(buttonRect(col, row));
            w_buttons[i].SetRes(&icons[i]);
            w_labels[i].SetRect(buttonTextRect(col, row));
            w_labels[i].set_font(resource_font(IDR_FNT_SMALL));
            w_labels[i].SetAlignment(Align_t::Center());
            w_labels[i].SetPadding({ 0, 0, 0, 0 });
            w_labels[i].SetText(_(labels[i]));
        }
    }

    filamentBtnSetState(MMU2::xState(marlin_vars()->mmu2_state.get()));

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

void screen_home_data_t::filamentBtnSetState(MMU2::xState mmu) {
    if (mmu != mmu_state) {
        mmu_state = mmu;

        // did not want to include MMU
        // it might be good idea to move mmu enum to extra header
        // TODO move this code
        switch (mmu_state) {
        case MMU2::xState::Active:
            w_buttons[buttonFilamentIndex].SetRes(&icons[iconMMUId]);
            w_buttons[buttonFilamentIndex].SetAction(FilamentBtnMMU_cb);
            w_buttons[buttonFilamentIndex].Unshadow();
            w_buttons[buttonFilamentIndex].Enable();
            break;
        case MMU2::xState::Connecting:
            w_buttons[buttonFilamentIndex].SetRes(&icons[iconMMUId]);
            if (w_buttons[buttonFilamentIndex].IsFocused()) {
                w_buttons[buttonFilamentIndex - 1].SetFocus();
            }
            w_buttons[buttonFilamentIndex].Shadow();
            w_buttons[buttonFilamentIndex].Disable();
            break;
        case MMU2::xState::Stopped:
            w_buttons[buttonFilamentIndex].SetRes(&icons[iconNonMMUId]);
            w_buttons[buttonFilamentIndex].SetAction(FilamentBtn_cb);
            w_buttons[buttonFilamentIndex].Unshadow();
            w_buttons[buttonFilamentIndex].Enable();
            break;
        }
    }
}

void screen_home_data_t::handle_crash_dump() {
    ::crash_dump::BufferT dump_buffer;
    const auto &present_dumps { ::crash_dump::get_present_dumps(dump_buffer) };
    if (present_dumps.size() == 0) {
        return;
    }
    if (MsgBoxWarning(_("Crash detected. Save it to USB?"), Responses_YesNo)
        == Response::Yes) {
        auto do_stage = [&](string_view_utf8 msg, std::invocable<const ::crash_dump::DumpHandler *> auto fp) {
            MsgBoxIconned box(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, std::move(msg), is_multiline::yes, &png::info_58x58);
            box.Show();
            draw();
            for (const auto &dump_handler : present_dumps) {
                fp(dump_handler);
            }
            box.Hide();
        };

        do_stage(_("Saving to USB"), [](const ::crash_dump::DumpHandler *handler) { handler->usb_save(); });
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

#if DEVELOPMENT_ITEMS()
    static size_t last_busy_reset_count = 0;
    size_t busy_reset_count1 = hw_i2c1_get_busy_clear_count();
    size_t busy_reset_count2 = hw_i2c2_get_busy_clear_count();
    size_t busy_reset_count3 = hw_i2c3_get_busy_clear_count();

    if (last_busy_reset_count != busy_reset_count1 + busy_reset_count2 + busy_reset_count3) {
        last_busy_reset_count = busy_reset_count1 + busy_reset_count2 + busy_reset_count3;
        const char txt[] = "I2C workaround applied\nI2C1: %d\nI2C2: %d\nI2C3: %d";
        std::array<char, sizeof(txt) + 19> buff; // leaves 2+6 digits per error .. max 99 999 999 error .. no way it would be so many
        snprintf(buff.begin(), buff.size(), txt, busy_reset_count1, busy_reset_count2, busy_reset_count3);
        MsgBoxWarning(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(buff.begin())), Responses_Ok);
    }
#endif

#if !DEVELOPER_MODE()
    #if PRINTER_IS_PRUSA_XL
    static bool first_time_check_st { true };
    if (first_time_check_st) {
        first_time_check_st = false;
        warn_unfinished_selftest_msgbox();
    }
    #endif

    handle_crash_dump();

    if (touch_broken_during_run) {
        static bool already_shown = false;
        if (!already_shown) {
            already_shown = true;
            MsgBoxWarning(_("Touch disabled. This feature is work-in-progress and is going to be fully available in a future update."), Responses_Ok);
        }
    }
#endif
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
        filamentBtnSetState(MMU2::xState(marlin_vars()->mmu2_state.get()));

#if HAS_SELFTEST()
        if (!DialogHandler::Access().IsOpen()) {
            // esp update has bigger priority tha one click print
            const auto fw_state = esp_fw_state();
            const bool esp_need_flash = fw_state == EspFwState::WrongVersion || fw_state == EspFwState::NoFirmware;
            if (try_esp_flash && esp_need_flash && netdev_is_enabled(NETDEV_ESP_ID)) {
                try_esp_flash = false; // do esp flash only once (user can press abort)
                marlin_gcode("M997 S1 O");
                return;
            } else {
                // on esp update, can use one click print
                if (GuiMediaEventsHandler::ConsumeOneClickPrinting()) {
                    // TODO this should be done in main thread before Event::MediaInserted is generated
                    // if it is not the latest gcode might not be selected
                    if (find_latest_gcode(
                            gui_media_SFN_path,
                            FILE_PATH_BUFFER_LEN,
                            gui_media_LFN,
                            FILE_NAME_BUFFER_LEN)) {
                        print_begin(gui_media_SFN_path, false);
                    }
                }
            }
        }
#endif // HAS_SELFTEST
    }

#if !HAS_LOADCELL()
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }
#endif

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
