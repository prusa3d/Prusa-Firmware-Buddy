// screen_home.cpp
#include "screen_home.hpp"
#include "stdio.h"
#include "file_raii.hpp"

#include "config.h"

#include "marlin_client.hpp"
#include "screen_filebrowser.hpp"
#include "print_utils.hpp"
#include "filename_type.hpp"
#include "settings_ini.hpp"
#include <wui_api.h>
#include <espif.h>

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "gui_media_events.hpp"
#include "DialogMoveZ.hpp"
#include "DialogHandler.hpp"
#include "img_resources.hpp"
#include "tasks.hpp"

#include "screen_printing.hpp"
#include "filament_sensors_handler.hpp"

#include "RAII.hpp"
#include "lazyfilelist.hpp"
#include "i18n.h"
#include "i2c.hpp"
#include "netdev.h"
#include "ini.h"

#include <option/has_loadcell.h>
#include <option/developer_mode.h>
#include <option/development_items.h>
#include <device/peripherals.h>
#include <option/has_mmu2.h>
#include <option/has_human_interactions.h>

#include "screen_menu_settings.hpp"
#include "screen_menu_filament.hpp"
#include "screen_menu_control.hpp"

#if HAS_MMU2()
    #include "screen_menu_filament_mmu.hpp"
#endif

#include <crash_dump/crash_dump_handlers.hpp>
#include "box_unfinished_selftest.hpp"
#include <transfers/transfer_file_check.hpp>
#include <guiconfig/guiconfig.h>

#include "usb_host.h"

// TODO remove netdev_is_enabled after it is defined
bool __attribute__((weak)) netdev_is_enabled([[maybe_unused]] const uint32_t netdev_id) { return true; }

bool screen_home_data_t::ever_been_opened = false;
bool screen_home_data_t::try_esp_flash = true;
bool screen_home_data_t::touch_broken_during_run = false;

#ifdef USE_ST7789
    #define GEN_ICON_NAMES(ICON) \
        { img::ICON##_64x64, img::ICON##_64x64_focused, img::ICON##_64x64_disabled }
#endif // USE_ST7789
#ifdef USE_ILI9488
    #define GEN_ICON_NAMES(ICON) \
        { img::ICON##_80x80, img::ICON##_80x80_focused, img::ICON##_80x80_disabled }
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
constexpr size_t buttonTextHeight = 13; // font_regular_7x13

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
    N_("Control"),
    N_("Settings"),
    N_("Info"),
    N_("No USB") // label variant for first button
};

bool screen_home_data_t::usbWasAlreadyInserted = false;
bool screen_home_data_t::need_check_wifi_credentials = true;

static void FilamentBtn_cb() {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilament>);
}

#if HAS_MMU2()
static void FilamentBtnMMU_cb() {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilamentMMU>);
}
#endif

// clang-format off
screen_home_data_t::screen_home_data_t()
    : AddSuperWindow<screen_t>()
    , usbInserted(marlin_vars()->media_inserted)
    , header(this)
    , footer(this)
#ifdef USE_ST7789
    , logo(this, logoRect, &img::printer_logo)
#endif // USE_ST7789
    , w_buttons {
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>); } },
        { this, Rect16(), nullptr, []() { marlin_client::gcode_printf("M1700 T-1"); } },
        { this, Rect16(), nullptr, FilamentBtn_cb },
        { this, Rect16(), nullptr, []() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuControl>); } },
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
    } {
    // clang-format on

    EnableLongHoldScreenAction();
    window_frame_t::ClrMenuTimeoutClose();
    window_frame_t::ClrOnSerialClose(); // don't close on Serial print
    WindowFileBrowser::SetRoot("/usb");

#ifndef USE_ST7789
    header.SetIcon(&img::home_shape_16x16);
#endif
#if !defined(_DEBUG) && !DEVELOPER_MODE()
    // regular home screen
    header.SetText(_("HOME"));

#else
    static const uint8_t msgHome[] = "HOME" TERN(DEVELOPER_MODE(), " - DEV", "") TERN(defined(_DEBUG), " - DEBUG", "");
    header.SetText(string_view_utf8::MakeCPUFLASH(msgHome)); // intentionally not translated
#endif

    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            const size_t i = row * 3 + col;
            w_buttons[i].SetRect(buttonRect(col, row));
            w_buttons[i].SetRes(&icons[i]);
            w_labels[i].SetRect(buttonTextRect(col, row));
            w_labels[i].set_font(Font::small);
            w_labels[i].SetAlignment(Align_t::Center());
            w_labels[i].SetPadding({ 0, 0, 0, 0 });
            w_labels[i].SetText(_(labels[i]));
        }
    }

    filamentBtnSetState();

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

void screen_home_data_t::filamentBtnSetState() {
#if HAS_MMU2()
    const MMU2::xState new_state = MMU2::xState(marlin_vars()->mmu2_state.get());
    if (new_state != mmu_state) {
        mmu_state = new_state;

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
        case MMU2::xState::Bootloader:
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
#endif
}

void screen_home_data_t::handle_crash_dump() {
    ::crash_dump::BufferT dump_buffer;
    const auto &present_dumps { ::crash_dump::get_present_dumps(dump_buffer) };
    if (present_dumps.size() == 0) {
        return;
    }
    if (MsgBoxWarning(_("Crash detected. Save it to USB?"
                        "\n\nDo not share the file publicly,"
                        " the crash dump may include unencrypted sensitive information."
                        " Send it to: reports@prusa3d.com"),
            Responses_YesNo)
        == Response::Yes) {
        auto do_stage = [&](string_view_utf8 msg, std::invocable<const ::crash_dump::DumpHandler *> auto fp) {
            MsgBoxIconned box(GuiDefaults::DialogFrameRect, Responses_NONE, 0, nullptr, std::move(msg), is_multiline::yes, &img::info_58x58);
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

#if !DEVELOPER_MODE()
    #if HAS_SELFTEST_SNAKE()
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

    #if 0 /// disable the warning for now but let's keep it around as we might want to reenable it for the public release
    static bool input_shaper_warning_shown = false;
    if (!input_shaper_warning_shown) {
        input_shaper_warning_shown = true;
        MsgBoxISWarning(_(
        #ifdef USE_ST7789
                            "This firmware is still\nin development.\n\n"
                            "Do not leave the printer unattended.\n\n"
        #else
                            "This firmware is still in development and is for testing purposes only.\n\n"
                            "Do not leave the printer unattended.\n\n"
        #endif
                            "More info at prusa.io/input-shaper"),
            Responses_Ok, 0, GuiDefaults::RectScreen);
    }
    #endif
#endif
}
namespace {
struct Config {
    enum class Status { missing,
        equal,
        not_equal };

    Status ssid_status = Status::missing;
    Status psk_status = Status::missing;

    Status get_status() {
        if (ssid_status == Status::missing || psk_status == Status::missing) {
            return Status::missing;
        } else if (ssid_status == Status::not_equal || psk_status == Status::not_equal) {
            return Status::not_equal;
        }
        return Status::equal;
    }
};

int ini_handler(void *user, const char *section, const char *name, const char *value) {
    if (user == nullptr || section == nullptr || name == nullptr || value == nullptr) {
        return 0;
    }

    if (strcmp("wifi", section)) {
        return 1; // do I return 0 or 1 ??? I have no clue what would 0 do.
    }

    auto *config = reinterpret_cast<Config *>(user);
    size_t len = strlen(value);

    if (strcmp(name, "ssid") == 0) {
        char buffer[config_store_ns::old_eeprom::WIFI_MAX_SSID_LEN];
        if (len <= sizeof(buffer)) {
            config->ssid_status = strncmp(value, config_store().wifi_ap_ssid.get_c_str(), sizeof(buffer)) ? Config::Status::not_equal : Config::Status::equal;
        }
    } else if (strcmp(name, "psk") == 0) {
        char buffer[config_store_ns::old_eeprom::WIFI_MAX_PASSWD_LEN];
        if (len <= sizeof(buffer)) {
            config->psk_status = strncmp(value, config_store().wifi_ap_password.get_c_str(), sizeof(buffer)) ? Config::Status::not_equal : Config::Status::equal;
        }
    }

    return 1;
}

Config::Status name_and_psk_status() {
    Config config;
    bool ok = ini_parse(settings_ini::file_name, ini_handler, &config) == 0;
    if (!ok) {
        return Config::Status::missing;
    }
    return config.get_status();
}
} // namespace

void screen_home_data_t::handle_wifi_credentials() {
    // first we find if there is an WIFI config
    bool has_wifi_credentials = false;
    {
        unique_file_ptr fl;
        // if other thread modifies files during this action, detection might fail
        fl.reset(fopen(settings_ini::file_name, "r"));
        has_wifi_credentials = fl.get() != nullptr;
    }
    if (has_wifi_credentials && (name_and_psk_status() == Config::Status::not_equal) && !option::developer_mode) {
        if (MsgBoxInfo(_("Wi-Fi credentials (SSID and password) discovered on the USB flash drive. Would you like to connect your printer to Wi-Fi now?"), Responses_YesNo, 1)
            == Response::Yes) {
            const auto fw_state = esp_fw_state();
            const bool esp_need_flash = fw_state == EspFwState::WrongVersion || fw_state == EspFwState::NoFirmware;
            if (esp_need_flash) {
                marlin_client::gcode("M997 S1"); // update esp, do not force update older fw
            } else {
                marlin_client::gcode("M1587"); // update esp credentials only
            }
            return;
        }
    }
}

void screen_home_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    // TODO: This easily freezes home screen when flash action fails to start.
    // There are several places in the code where executing a flash gcode can
    // result in no-op and home screen stays active with events disabled.
    if (event == GUI_event_t::MEDIA) { // Also stores during windowEvent recursion
        media_event = MediaState_t(int(param));
    }

    if (event_in_progress) {
        return;
    }

    AutoRestore avoid_recursion(event_in_progress, true);

    on_enter();

    if (media_event != MediaState_t::unknown) {
        switch (MediaState_t(media_event)) {
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
        media_event = MediaState_t::unknown;
    }

    if (event == GUI_event_t::LOOP) {
        filamentBtnSetState();

#if ENABLED(POWER_PANIC)
        if (TaskDeps::check(TaskDeps::Dependency::usb_and_temp_ready) && !power_panic::is_power_panic_resuming())
#endif // ENABLED(POWER_PANIC)
        { // every time usb is inserted we check wifi credentials
            if (usbInserted) {
                if (need_check_wifi_credentials) {
                    need_check_wifi_credentials = false;
                    handle_wifi_credentials();
                }
            } else {
                need_check_wifi_credentials = true; // usb is not inserted, when it gets inserted we want to recheck credentials file
            }
        }

#if HAS_SELFTEST()
        if (!DialogHandler::Access().IsOpen()) {
            // esp update has bigger priority tha one click print
            const auto fw_state = esp_fw_state();
            const bool esp_need_flash = fw_state == EspFwState::WrongVersion || fw_state == EspFwState::NoFirmware;
            if (try_esp_flash && esp_need_flash && netdev_is_enabled(NETDEV_ESP_ID) && !option::developer_mode) {
                try_esp_flash = false; // do esp flash only once (user can press abort)
                marlin_client::gcode("M997 S1 O");
                return;
            } else {
                // on esp update, can use one click print
                if (HAS_HUMAN_INTERACTIONS() &&
    #if ENABLED(POWER_PANIC)
                    TaskDeps::check(TaskDeps::Dependency::usb_and_temp_ready) && !power_panic::is_power_panic_resuming() &&
    #endif // ENABLED(POWER_PANIC)
                    GuiMediaEventsHandler::ConsumeOneClickPrinting() && !usbh_power_cycle::block_one_click_print()) {
                    // TODO this should be done in main thread before Event::MediaInserted is generated
                    // if it is not the latest gcode might not be selected
                    if (find_latest_gcode(
                            gui_media_SFN_path,
                            FILE_PATH_BUFFER_LEN,
                            gui_media_LFN,
                            FILE_NAME_BUFFER_LEN)) {
                        print_begin(gui_media_SFN_path);
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
    strlcpy(fpath, "/usb", fpath_len);

    F_DIR_RAII_Iterator dir(fpath);
    if (dir.result == ResType::NOK) {
        return false;
    }

    // prepare the item at the zeroth position according to sort policy
    FileSort::Entry entry;

    while (dir.FindNext()) {
        const FileSort::EntryRef curr(*dir.fno, fpath);

        if (curr.type != FileSort::EntryType::FILE) {
            continue;
        }

        if (entry.is_valid() && !FileSort::less_by_time(curr, entry)) {
            continue;
        }

        entry.CopyFrom(curr);
    }

    if (!entry.is_valid()) {
        return false;
    }

    fpath[4] = '/';
    strlcpy(fpath + 5, entry.sfn, fpath_len - 5);
    strlcpy(fname, entry.lfn, fname_len);
    return true;
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
    if (!var.GetPosition()) {
        return;
    }

    size_t pos = *(var.GetPosition());
    if (pos >= button_count) {
        return;
    }

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
