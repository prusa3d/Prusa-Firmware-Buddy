#include <stdio.h>
#include "client_fsm_types.h"
#include "gui_time.hpp"
#include "gui.hpp"
#include "config.h"
#include "marlin_client.hpp"
#include "display.h"
#include "display_hw_checks.hpp"
#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "window_file_list.hpp"
#include "window_header.hpp"
#include "window_dlg_wait.hpp"
#include "window_dlg_popup.hpp"
#include "window_dlg_strong_warning.hpp"
#include "window_dlg_preheat.hpp"
#include "screen_print_preview.hpp"
#include "screen_hardfault.hpp"
#include "screen_qr_error.hpp"
#include "screen_watchdog.hpp"
#include "screen_bsod.hpp"
#include "screen_stack_overflow.hpp"
#include "screen_filebrowser.hpp"
#include "screen_printing.hpp"
#include "gui_bootstrap_screen.hpp"
#include "IScreenPrinting.hpp"
#include "DialogHandler.hpp"
#include "sound.hpp"
#include "str_utils.hpp"
#include "knob_event.hpp"
#include "DialogMoveZ.hpp"
#include "ScreenShot.hpp"
#include "i18n.h"
#include "w25x.h"
#include "../mmu2/mmu2_error_converter.h"
#include "../../Marlin/src/feature/prusa/MMU2/mmu2_progress_converter.h"
#include "screen_home.hpp"
#include "gui_fsensor_api.hpp"
#include "tasks.hpp"
#include "timing.h"
#include "gcode_info.hpp"
#include "version.h"
#include "touch_get.hpp"
#include "touch_dependency.hpp"
#include "language_eeprom.hpp"

#include <option/has_side_leds.h>

#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    #include "hw_configuration.hpp"
#endif

#if HAS_SELFTEST()
    #include "ScreenSelftest.hpp"
#endif

#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif

#include "gpio.h"
#include "Jogwheel.hpp"
#include "hwio.h"
#include "sys.h"
#include "wdt.h"
#include <crash_dump/dump.hpp>
#include "gui_media_events.hpp"
#include "metric.h"
#include "neopixel.hpp"
#include "led_lcd_cs_selector.hpp"
#include "gui_leds.hpp"
#include "hwio_pindef.h"
#include "main.h"
#include "bsod.h"
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <option/has_leds.h>
#if HAS_LEDS()
    #include "led_animations/printer_animation_state.hpp"
#endif
#include "log.h"
#include <printers.h>

#include <option/has_selftest_snake.h>
#if HAS_SELFTEST_SNAKE()
    #include "screen_menu_selftest_snake.hpp"
#endif

#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX
    #include "MItem_love_board.hpp"
#endif

#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    #include "menu_item_xlcd.hpp"
#endif

#include <config_store/store_instance.hpp>

using namespace buddy::hw;

LOG_COMPONENT_REF(GUI);
LOG_COMPONENT_REF(Buddy);
LOG_COMPONENT_DEF(XLCD, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(LoveBoard, LOG_SEVERITY_INFO);
extern void blockISR(); // do not want to include marlin temperature

#ifdef USE_ST7789
const st7789v_config_t st7789v_cfg = {
    .phspi = &hspi2, // spi handle pointer
    .flg = ST7789V_FLG_DMA, // flags (DMA, MISO)
    .colmod = ST7789V_DEF_COLMOD, // interface pixel format (5-6-5, hi-color)
    .madctl = ST7789V_DEF_MADCTL, // memory data access control (no mirror XY)
    .gamma = 0,
    .brightness = 0,
    .is_inverted = 0,
    .control = 0,
};
#endif // USE_ST7789

#ifdef USE_ILI9488
const ili9488_config_t ili9488_cfg = {
    .phspi = &SPI_HANDLE_FOR(lcd), // spi handle pointer
    .flg = ILI9488_FLG_DMA, // flags (DMA, MISO)
    .colmod = ILI9488_DEF_COLMOD, // interface pixel format (5-6-5, hi-color)
    .madctl = ILI9488_DEF_MADCTL, // memory data access control (no mirror XY)
    .gamma = 0,
    .brightness = 0,
    .is_inverted = 0,
    .control = 0,
    .pwm_inverted = 0b10110001 // inverted
};
#endif // USE_ILI9488

marlin_vars_t *gui_marlin_vars = 0;

char gui_media_LFN[FILE_NAME_BUFFER_LEN];
char gui_media_SFN_path[FILE_PATH_BUFFER_LEN]; //@@TODO DR - tohle pouzit na ulozeni posledni cesty

#ifdef GUI_JOGWHEEL_SUPPORT
Jogwheel jogwheel;
#endif // GUI_JOGWHEEL_SUPPORT

MsgBuff_t &MsgCircleBuffer() {
    static CircleStringBuffer<MSG_STACK_SIZE, MSG_MAX_LENGTH> ret;
    return ret;
}

void MsgCircleBuffer_cb(const char *txt) {
    MsgCircleBuffer().push_back(txt);
    // cannot open == already opened
    IScreenPrinting *const prt_screen = IScreenPrinting::GetInstance();
    if (prt_screen && (!prt_screen->GetPopUpRect().IsEmpty())) {
        // message for MakeRAM must exist at least as long as string_view_utf8 exists
        static std::array<uint8_t, MSG_MAX_LENGTH> msg;
        strlcpy((char *)msg.data(), txt, MSG_MAX_LENGTH);
        window_dlg_popup_t::Show(prt_screen->GetPopUpRect(), string_view_utf8::MakeRAM(msg.data()), POPUP_MSG_DUR_MS);
    }
}

void Warning_cb(WarningType type) {
    switch (type) {
    case WarningType::HotendFanError:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::HotendFan);
        break;
    case WarningType::PrintFanError:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::PrintFan);
        break;
    case WarningType::HotendTempDiscrepancy:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::HotendTempDiscrepancy);
        break;
    case WarningType::HeatersTimeout:
    case WarningType::NozzleTimeout:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::HeatersTimeout);
        break;
#if _DEBUG
    case WarningType::SteppersTimeout:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::SteppersTimeout);
        break;
#endif
    case WarningType::USBFlashDiskError:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::USBFlashDisk);
        break;
    case WarningType::HeatBreakThermistorFail:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::HBThermistorFail);
        break;
#if ENABLED(POWER_PANIC)
    case WarningType::HeatbedColdAfterPP:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::HeatbedColdAfterPP);
        break;
#endif
    case WarningType::NozzleDoesNotHaveRoundSection:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::NozzleDoesNotHaveRoundSection);
        break;
    case WarningType::NotDownloaded:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::NotDownloaded);
        break;
    case WarningType::BuddyMCUMaxTemp:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::BuddyMCUMaxTemp);
        break;
#if HAS_DWARF()
    case WarningType::DwarfMCUMaxTemp:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::DwarfMCUMaxTemp);
        break;
#endif /* HAS_DWARF() */
#if HAS_MODULARBED()
    case WarningType::ModBedMCUMaxTemp:
        window_dlg_strong_warning_t::ShowType(window_dlg_strong_warning_t::ModBedMCUMaxTemp);
        break;
#endif /* HAS_MODULARBED() */
    default:
        break;
    }
}

static void Startup_cb(void) {
}

namespace {
void led_animation_step() {
#if HAS_LEDS()
    PrinterStateAnimation::Update();
    Animator_LCD_leds().Step();
    leds::TickLoop();
#endif
}

void filament_sensor_validation() {
    if (screen_home_data_t::EverBeenOpened()
#if HAS_SELFTEST()
    #if HAS_SELFTEST_SNAKE()
        && !Screens::Access()->IsScreenOnStack<ScreenMenuSTSWizard>()
        && !Screens::Access()->IsScreenOnStack<ScreenMenuSTSCalibrations>()
    #else
        && (ScreenSelftest::GetInstance() == nullptr)
    #endif
#endif
    ) {
        // GuiFSensor::validate_for_cyclical_calls(); // removed for now, unfinished calibration will be sufficient.
    }
}

void make_gui_ready_to_print() {
    /**
     * This function is triggered because of marlin_server::State::WaitGui and it is checking if GUI thread is safe to start printing.
     * State::WaitGui is set, when print_begin() is passed to marlin_server from any of marlin_clients (from GUI / Connect / pLink etc...)
     *
     * Here, we're checking from GUI thread, if print can be started in current GUI state,
     * it is not allowed when some FSM is opened, the FSMs that can be printed from are:
     *   Printing screen (reprint)
     *   Both Print previews (from filebrowser and from one-click) - calling print from internet, while PrintPreview in GUI is opened
     */

    // We don't want any FSM opened - for example LoadUnload could invade this logic
    bool can_print_on_current_screen = !DialogHandler::Access().IsAnyOpen();

    // Handle unusual usecase when print preview is already open, but print is called from Connect / pLink
    bool one_click_preview = Screens::Access()->Count() == 1 && Screens::Access()->IsScreenOpened<ScreenPrintPreview>();
    bool filebrowser_preview = Screens::Access()->Count() == 2 && Screens::Access()->IsScreenOpened<ScreenPrintPreview>() && Screens::Access()->IsScreenOnStack<screen_filebrowser_data_t>();

    if (can_print_on_current_screen || one_click_preview || filebrowser_preview) {
        {
            // Update printed filename from marlin_server, sample LFN+SFN atomically
            auto lock = MarlinVarsLockGuard();
            marlin_vars()->media_LFN.copy_to(gui_media_LFN, sizeof(gui_media_LFN), lock);
            marlin_vars()->media_SFN_path.copy_to(gui_media_SFN_path, sizeof(gui_media_SFN_path), lock);
        }

        // Handle different states of GUI before print begins
        if (can_print_on_current_screen) {
            bool have_file_browser = Screens::Access()->IsScreenOnStack<screen_filebrowser_data_t>();
            Screens::Access()->ClosePrinting(); // set flag to close all appropriate screens
            if (have_file_browser) {
                Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>);
                Screens::Access()->Get()->Validate(); // Do not draw filebrowser now
            }
            Screens::Access()->Loop(); // close those screens before marlin_gui_ready_to_print
            marlin_client::marlin_gui_ready_to_print(); // notify server, that GUI is ready to print
        } else if (one_click_preview || filebrowser_preview) {
            // Print is called from Connect/pLink, while print preview is already open in GUI
            // notify server, that GUI is ready to print
            marlin_client::marlin_gui_ready_to_print();
        }
        // else not reachable

        Screens::Access()->Get()->Validate(); // Do not redraw after CloseAll (keep wait dialog displayed)

        while (!DialogHandler::Access().IsAnyOpen() // Wait for start of the print - to prevent any unwanted GUI action
            && marlin_vars()->print_state != marlin_server::State::Idle) { // Abort if print was not started (this function is called when State::WaitGui)
            // main thread is processing a print
            // wait for print screen to open, any fsm can break waiting (f.e.: Print Preview)
            gui_timers_cycle(); // refresh GUI time
            marlin_client::loop(); // refresh fsm - required for dialog handler
            DialogHandler::Access().Loop();
        }

    } else {
        // Do not print on current screen -> main thread will set printer_state to Idle
        marlin_client::marlin_gui_cant_print();
    }
}
} // anonymous namespace

static void log_onewire_otp() {
#if DEVELOPMENT_ITEMS()
    #if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX
    OtpStatus loveboard = buddy::hw::Configuration::Instance().get_loveboard_status();

    if (loveboard.data_valid) {
        log_info(LoveBoard, "PASSED: Read e. %u, Repeated e. %u, Cyclic e. %u, Retried %u",
            loveboard.single_read_error_counter, loveboard.repeated_read_error_counter, loveboard.cyclic_read_error_counter, loveboard.retried);
        log_info(LoveBoard, "Eeprom: %s", MI_INFO_SERIAL_NUM_LOVEBOARD::to_array().data());
    } else {
        log_error(LoveBoard, "FAILED: Read e. %u, Repeated e. %u, Cyclic e. %u, Retried %u",
            loveboard.single_read_error_counter, loveboard.repeated_read_error_counter, loveboard.cyclic_read_error_counter, loveboard.retried);
    }
    #endif

    #if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    OtpStatus xlcd = buddy::hw::Configuration::Instance().get_xlcd_status();

    log_info(XLCD, "%s: Read e. %u, Repeated e. %u, Cyclic e. %u, Retried %u",
        xlcd.data_valid ? "DETECTED" : "NOT DT.", xlcd.single_read_error_counter, xlcd.repeated_read_error_counter, xlcd.cyclic_read_error_counter, xlcd.retried);

    if (xlcd.data_valid) {
        log_info(XLCD, "Eeprom: %s", MI_INFO_SERIAL_NUM_XLCD::to_array().data());
    }
    #endif

#endif
}

/**
 * @brief Get the right error page to display
 *
 * Error has precedence over dump.
 */
static ScreenFactory::Creator get_error_screen() {
    if (crash_dump::message_get_type() == crash_dump::MsgType::RSOD && !crash_dump::message_is_displayed()) {
        return ScreenFactory::Screen<ScreenErrorQR>;
    }

    if (crash_dump::dump_is_valid() && !crash_dump::dump_is_displayed()) {
        if (crash_dump::message_is_displayed()) {
            // In case message is stale (already displayed), it is not relevant anymore.
            // We have just crash dump without message. CrashDump without message means it was caused by hardfault directly.
            return ScreenFactory::Screen<ScreenHardfault>;
        }

        switch (crash_dump::message_get_type()) {
        case crash_dump::MsgType::IWDGW:
            return ScreenFactory::Screen<ScreenWatchdog>;
        case crash_dump::MsgType::BSOD:
            return ScreenFactory::Screen<ScreenBsod>;
        case crash_dump::MsgType::STACK_OVF:
            return ScreenFactory::Screen<ScreenStackOverflow>;
        default:
            break;
        }
    }

    // Display an unknown error page
    return ScreenFactory::Screen<ScreenBlueError>;
}

void gui_error_run(void) {
#ifdef USE_ST7789
    st7789v_config = st7789v_cfg;
#endif

#ifdef USE_ILI9488
    ili9488_config = ili9488_cfg;
#endif
    gui_init();

    screen_node screen_initializer { get_error_screen() };
    Screens::Init(screen_initializer);

    // Mark everything as displayed
    crash_dump::message_set_displayed();
    crash_dump::dump_set_displayed();

#if HAS_LEDS()
    leds::Init();
#endif

    LangEEPROM::getInstance(); // Initialize language EEPROM value

    while (true) {
        gui::StartLoop();

#if HAS_LEDS()
        PrinterStateAnimation::Update();
        Animator_LCD_leds().Step();
        leds::TickLoop();
#endif

        Screens::Access()->Loop();
        gui_bare_loop();
        gui::EndLoop();
    }
}

void gui_run(void) {
#ifdef USE_ST7789
    st7789v_config = st7789v_cfg;
#endif

#ifdef USE_ILI9488
    ili9488_config = ili9488_cfg;
#endif

    gui_init();

    gui::knob::RegisterHeldLeftAction(TakeAScreenshot);
    gui::knob::RegisterLongPressScreenAction(DialogMoveZ::Show);

    screen_node screen_initializer[] {
        ScreenFactory::Screen<screen_splash_data_t>, // splash
        ScreenFactory::Screen<screen_home_data_t> // home
    };
    Screens::Init(screen_initializer, screen_initializer + (sizeof(screen_initializer) / sizeof(screen_initializer[0])));

    // TIMEOUT variable getting value from EEPROM when EEPROM interface is initialized
    if (config_store().menu_timeout.get()) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }

    Screens::Access()->Loop();
#if HAS_LEDS()
    leds::Init();
#endif
    // Show bootstrap screen untill firmware initializes
    gui_bootstrap_screen_run();

    marlin_client::init();
    GCodeInfo::getInstance().Init(gui_media_LFN, gui_media_SFN_path);

    DialogHandler::Access(); // to create class NOW, not at first call of one of callback
    marlin_client::set_fsm_cb(DialogHandler::command_c_compatible);
    marlin_client::set_message_cb(MsgCircleBuffer_cb);
    marlin_client::set_warning_cb(Warning_cb);
    marlin_client::set_startup_cb(Startup_cb);

    Sound_Play(eSOUND_TYPE::Start);

    marlin_client::set_event_notify(marlin_server::EVENT_MSK_DEF, nullptr);

    // Close bootstrap screen, open home screen
    Screens::Access()->Close();

#if HAS_LEDS() && !HAS_SIDE_LEDS()
    // we need to step the animator, to move the started animation to current to let it run for one cycle
    auto guard = leds::start_animation(PrinterState::PowerUp, 10);
    Animator_LCD_leds().Step();
    guard.Stop();
#endif

#if HAS_SIDE_LEDS()
    leds::side_strip_control.ActivityPing();
#endif

    log_onewire_otp();

    // TODO make some kind of registration
    while (1) {
        gui::StartLoop();

        led_animation_step();

        filament_sensor_validation();

        lcd::communication_check();

        // I must do it before screen and dialog loops
        // do not use marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE))->print_state, it can make gui freeze in case main thread is unresponsive
        volatile bool print_processor_waiting = marlin_vars()->print_state == marlin_server::State::WaitGui;

        DialogHandler::Access().Loop();

        // this code handles start of print
        // it must be in main gui loop just before screen handler to ensure no FSM is opened
        // !DialogHandler::Access().IsAnyOpen() - wait until all FSMs are closed (including one click print)
        if (print_processor_waiting) {
            make_gui_ready_to_print();
        }

        Screens::Access()->Loop();
        gui_loop();
        gui::EndLoop();
    }
}
