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
#include "screen_unknown.hpp"
#include "window_file_list.hpp"
#include "window_header.hpp"
#include "window_temp_graph.hpp"
#include "window_dlg_wait.hpp"
#include "window_dlg_popup.hpp"
#include "window_dlg_strong_warning.hpp"
#include "window_dlg_preheat.hpp"
#include "screen_print_preview.hpp"
#include "screen_hardfault.hpp"
#include "screen_qr_error.hpp"
#include "screen_watchdog.hpp"
#include "screen_filebrowser.hpp"
#include "screen_printing.hpp"
#include "IScreenPrinting.hpp"
#include "DialogHandler.hpp"
#include "sound.hpp"
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

#include <option/bootloader.h>
#include <option/bootloader_update.h>
#include <option/resources.h>
#include <option/has_puppies.h>
#include <option/has_puppies_bootloader.h>
#include <option/has_side_leds.h>
#include <option/has_embedded_esp32.h>

#if HAS_SELFTEST()
    #include "ScreenSelftest.hpp"
#endif

#if ENABLED(RESOURCES())
    #include "resources/bootstrap.hpp"
    #include "resources/revision_standard.hpp"
#endif
#if ENABLED(HAS_PUPPIES())
    #include "puppies/PuppyBootstrap.hpp"
    #include "puppies/puppy_task.hpp"
using buddy::puppies::PuppyBootstrap;
#endif
#if BOTH(RESOURCES(), BOOTLOADER())
    #include "bootloader/bootloader.hpp"
#endif

#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif

int guimain_spi_test = 0;

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
#if HAS_LEDS
    #include "led_animations/printer_animation_state.hpp"
#endif
#include "log.h"
#include <esp_flash.hpp>
#include <printers.h>

#include <option/has_selftest_snake.h>
#if HAS_SELFTEST_SNAKE()
    #include "screen_menu_selftest_snake.hpp"
#endif

#include <configuration_store.hpp>

using namespace buddy::hw;

LOG_COMPONENT_REF(GUI);
LOG_COMPONENT_REF(Buddy);
extern void blockISR(); // do not want to include marlin temperature

#ifdef USE_ST7789
const st7789v_config_t st7789v_cfg = {
    .phspi = &hspi2,              // spi handle pointer
    .flg = ST7789V_FLG_DMA,       // flags (DMA, MISO)
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
    .flg = ILI9488_FLG_DMA,        // flags (DMA, MISO)
    .colmod = ILI9488_DEF_COLMOD,  // interface pixel format (5-6-5, hi-color)
    .madctl = ILI9488_DEF_MADCTL,  // memory data access control (no mirror XY)
    .gamma = 0,
    .brightness = 0,
    .is_inverted = 0,
    .control = 0
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
        window_dlg_popup_t::Show(prt_screen->GetPopUpRect(), string_view_utf8::MakeRAM(msg.data()), 5000);
    }
}

void Warning_cb(WarningType type) {
    switch (type) {
    case WarningType::HotendFanError:
        window_dlg_strong_warning_t::ShowHotendFan();
        break;
    case WarningType::PrintFanError:
        window_dlg_strong_warning_t::ShowPrintFan();
        break;
    case WarningType::HotendTempDiscrepancy:
        window_dlg_strong_warning_t::ShowHotendTempDiscrepancy();
        break;
    case WarningType::HeatersTimeout:
    case WarningType::NozzleTimeout:
        window_dlg_strong_warning_t::ShowHeatersTimeout();
        break;
#if DEVELOPMENT_ITEMS()
    case WarningType::SteppersTimeout:
        window_dlg_strong_warning_t::ShowSteppersTimeout();
        break;
#endif
    case WarningType::USBFlashDiskError:
        window_dlg_strong_warning_t::ShowUSBFlashDisk();
        break;
    case WarningType::HeatBreakThermistorFail:
        window_dlg_strong_warning_t::ShowHeatBreakThermistorFail();
        break;
#if ENABLED(POWER_PANIC)
    case WarningType::HeatbedColdAfterPP:
        window_dlg_strong_warning_t::ShowHeatbedColdAfterPP();
        break;
#endif
    default:
        break;
    }
}

static void Startup_cb(void) {
}

void client_gui_refresh() {
    static uint32_t start = gui::GetTick_ForceActualization();
    static uint32_t last_tick = gui::GetTick_ForceActualization();
    uint32_t tick = gui::GetTick_ForceActualization();
    if (last_tick != tick) {
        unsigned percent = (tick - start) / (1000 / 100); // 1000ms / 100%
        percent = ((percent < 99) ? percent : 99);

#if PRINTER_IS_PRUSA_XL
        // XL has a puppy bootstrap which ends the progress bar at buddy::puppies::max_bootstrap_perc
        percent = buddy::puppies::max_bootstrap_perc + percent * buddy::puppies::max_bootstrap_perc / 100;
#elif BOOTLOADER()
        // when running under bootloader, we take over the progress bar at 50 %
        percent = 50 + percent / 2;
#endif

        GUIStartupProgress progr = { unsigned(percent), nullptr };
        event_conversion_union un;
        un.pGUIStartupProgress = &progr;
        Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);

        last_tick = tick;
        gui_redraw();
    }
}

namespace {
void led_animation_step() {
#if HAS_LEDS
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
        GuiFSensor::validate_for_cyclical_calls();
    }
}

// Callback for wait dialog - initialize the new g-code
void gcode_file_init() {

    GCodeInfo::getInstance().initFile(GCodeInfo::GI_INIT_t::FULL); // Parse (this takes a while)
    // Close it after initFile is completed, so that it runs only once
    Screens::Access()->Close(); // close-flag this dialog
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
        // Parse the printed file for preview info
        {
            // Update printed filename from marlin_server, sample LFN+SFN atomically
            auto lock = MarlinVarsLockGuard();
            marlin_vars()->media_LFN.copy_to(gui_media_LFN, sizeof(gui_media_LFN), lock);
            marlin_vars()->media_SFN_path.copy_to(gui_media_SFN_path, sizeof(gui_media_SFN_path), lock);
        }
        gui_dlg_wait(gcode_file_init); // This will initialize new g-code file

        // Handle different states of GUI before print begins
        if (can_print_on_current_screen) {
            bool have_file_browser = Screens::Access()->IsScreenOnStack<screen_filebrowser_data_t>();
            Screens::Access()->ClosePrinting(); // set flag to close all appropriate screens
            if (have_file_browser) {
                Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>);
                Screens::Access()->Get()->Validate(); // Do not draw filebrowser now
            }
            Screens::Access()->Loop();                // close those screens before marlin_gui_ready_to_print
            marlin_gui_ready_to_print();              // notify server, that GUI is ready to print
        } else if (one_click_preview || filebrowser_preview) {
            // Print is called from Connect/pLink, while print preview is already open in GUI
            // notify server, that GUI is ready to print
            marlin_gui_ready_to_print();
        }
        // else not reachable

        Screens::Access()->Get()->Validate(); // Do not redraw after CloseAll (keep wait dialog displayed)
        // wait for start of the print - to prevent any unwanted GUI action
        while (!DialogHandler::Access().IsAnyOpen()) {
            // main thread is processing a print
            // wait for print screen to open, any fsm can break waiting (f.e.: Print Preview)
            gui_timers_cycle();   // refresh GUI time
            marlin_client_loop(); // refresh fsm - required for dialog handler
            DialogHandler::Access().Loop();
        }

    } else {
        // Do not print on current screen -> main thread will set printer_state to Idle
        marlin_gui_cant_print();
    }
}
} // anonymous namespace

#if ENABLED(RESOURCES())
static void finish_update() {

    #if ENABLED(BOOTLOADER_UPDATE())
    if (buddy::bootloader::needs_update()) {
        buddy::bootloader::update(
            [](int percent_done, buddy::bootloader::UpdateStage stage) {
                const char *stage_description;
                switch (stage) {
                case buddy::bootloader::UpdateStage::LookingForBbf:
                    stage_description = "Looking for BBF...";
                    break;
                case buddy::bootloader::UpdateStage::PreparingUpdate:
                case buddy::bootloader::UpdateStage::Updating:
                    stage_description = "Updating bootloader";
                    break;
                default:
                    bsod("unreachable");
                }

                log_info(Buddy, "Bootloader update progress %s (%i %%)", stage_description, percent_done);
                screen_splash_data_t::bootstrap_cb(percent_done, stage_description);
                gui_redraw();
            });
    }
    #endif

    if (!buddy::resources::has_resources(buddy::resources::revision::standard)) {
        buddy::resources::bootstrap(
            buddy::resources::revision::standard, [](int percent_done, buddy::resources::BootstrapStage stage) {
                const char *stage_description;
                switch (stage) {
                case buddy::resources::BootstrapStage::LookingForBbf:
                    stage_description = "Looking for BBF...";
                    break;
                case buddy::resources::BootstrapStage::PreparingBootstrap:
                    stage_description = "Preparing";
                    break;
                case buddy::resources::BootstrapStage::CopyingFiles:
                    stage_description = "Installing";
                    break;
                default:
                    bsod("unreachable");
                }
                log_info(Buddy, "Bootstrap progress %s (%i %%)", stage_description, percent_done);
                screen_splash_data_t::bootstrap_cb(percent_done, stage_description);
                gui_redraw();
            });
    }
    TaskDeps::provide(TaskDeps::Dependency::resources_ready);
}
#endif

#if BOARD_VER_EQUAL_TO(0, 5, 0)
// This is temporary, remove once everyone has compatible hardware.
// Requires new sandwich rev. 06 or rev. 05 with R83 removed.

    #if HAS_EMBEDDED_ESP32()
static void finish_update_ESP32() {
    // Show ESP flash progress
    while (TaskDeps::check(TaskDeps::Dependency::esp_flashed) == false) {
        const ESPFlash::Progress progress = ESPFlash::get_progress();
        const uint8_t percent = progress.bytes_total ? 100 * progress.bytes_flashed / progress.bytes_total : 0;
        const char *description;
        switch (progress.state) {
        case ESPFlash::State::Init:
            description = "Connecting ESP";
            break;
        case ESPFlash::State::WriteData:
            description = "Flashing ESP";
            break;
        case ESPFlash::State::Checking:
            description = "Checking ESP";
            break;
        default:
            description = "Unknown ESP state";
        }
        screen_splash_data_t::bootstrap_cb(percent, description);

        gui_redraw();
        osDelay(20);
    }
}
    #endif
#endif

#if ENABLED(HAS_PUPPIES())
static void finish_update_puppies() {
    // wait for puppies to become available
    while (TaskDeps::check(TaskDeps::Dependency::puppies_ready) == false) {
        auto progress = buddy::puppies::get_bootstrap_progress();
        if (progress.has_value()) {
            screen_splash_data_t::bootstrap_cb(progress->percent_done * buddy::puppies::max_bootstrap_perc / 100, progress->description());
            gui_redraw();
        }
        osDelay(20);
    }
}
#endif

constexpr size_t strlen_constexpr(const char *str) {
    return *str ? 1 + strlen_constexpr(str + 1) : 0;
}

/**
 * @brief Bootstrap finished
 *
 * Report bootstrap finished and firmware version.
 * This needs to be called after resources were successfully updated
 * in xFlash. This also needs to be called even if xFlash / resources
 * are unused. This needs to be output to standard USB CDC destination.
 * Format of the messages can not be changed as test station
 * expect those as step in manufacturing process.
 * The board needs to be able to report this with no additional
 * dependencies to connected peripherals.
 *
 * It is expected, that the testing station opens printer's serial port at 115200 bauds to obtain these messages.
 * Beware: previous attempts to writing these messages onto USB CDC log destination (baudrate 57600) resulted
 * in cross-linked messages because the logging subsystem intentionally has no prevention (locks/mutexes) against such a situation.
 * Therefore the only reliable output is the "Marlin's" serial output (before Marlin is actually started)
 * as nothing else is actually using this serial line (therefore no cross-linked messages can appear at this spot),
 * and Marlin itself is guaranteed to not have been started due to dependency USBSERIAL_READY.
 */
static void manufacture_report() {
    // The first '\n' is just a precaution - terminate any partially printed message from Marlin if any
    static const uint8_t intro[] = "\nbootstrap finished\nfirmware version: ";

    static_assert(sizeof(intro) > 1);          // prevent accidental buffer underrun below
    SerialUSB.write(intro, sizeof(intro) - 1); // -1 prevents from writing the terminating \0 onto the serial line
    SerialUSB.write(reinterpret_cast<const uint8_t *>(project_version_full), strlen_constexpr(project_version_full));
    SerialUSB.write('\n');

    TaskDeps::provide(TaskDeps::Dependency::manufacture_report_sent);
}

/**
 * @brief Get the right error page to display
 *
 * Error has precedence over dump.
 */
static ScreenFactory::Creator get_error_screen() {
    if (crash_dump::dump_err_in_xflash_is_valid() && !crash_dump::dump_err_in_xflash_is_displayed()) {
        return ScreenFactory::Screen<ScreenErrorQR>;
    }

    if (crash_dump::dump_in_xflash_is_valid() && !crash_dump::dump_in_xflash_is_displayed()) {
        switch (crash_dump::dump_in_xflash_get_type()) {
        case crash_dump::DumpType::DUMP_HARDFAULT:
            return ScreenFactory::Screen<screen_hardfault_data_t>;
#ifndef _DEBUG
        case crash_dump::DumpType::DUMP_IWDGW:
            return ScreenFactory::Screen<screen_watchdog_data_t>;
#endif
        default:
            return ScreenFactory::Screen<screen_unknown_data_t>;
        }
    }

    // Do not display nullptr page in case of all previous fails
    // Display an unknown error page instead
    // TODO: This is not perfect, would be better to be sure there is some error to display.
    return ScreenFactory::Screen<screen_unknown_data_t>;
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
    crash_dump::dump_err_in_xflash_set_displayed();
    crash_dump::dump_in_xflash_set_displayed();

#if HAS_LEDS
    // Turn on LCD backlight
    // TODO move SetBrightness to display
    leds::SetBrightness(100);
    leds::TickLoop();
#endif

    while (true) {
        gui::StartLoop();

#if HAS_LEDS
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
        ScreenFactory::Screen<screen_home_data_t>    // home
    };

    // Screens::Init(ScreenFactory::Screen<screen_splash_data_t>);
    Screens::Init(screen_initializer, screen_initializer + (sizeof(screen_initializer) / sizeof(screen_initializer[0])));

    // TIMEOUT variable getting value from EEPROM when EEPROM interface is initialized
    if (config_store().menu_timeout.get()) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }

    Screens::Access()->Loop();

#if HAS_LEDS
    // Turn on LCD backlight
    // TODO move SetBrightness to display
    leds::SetBrightness(100);
    leds::TickLoop();
#endif

#if ENABLED(RESOURCES())
    finish_update();
#endif

    manufacture_report();

    // Postpone starting Marlin after USBSerial handling in manufacture_report()
    TaskDeps::provide(TaskDeps::Dependency::usbserial_ready);

#if BOARD_VER_EQUAL_TO(0, 5, 0)
    // This is temporary, remove once everyone has compatible hardware.
    // Requires new sandwich rev. 06 or rev. 05 with R83 removed.

    #if HAS_EMBEDDED_ESP32()
    finish_update_ESP32();
    #endif
#endif

#if ENABLED(HAS_PUPPIES())
    finish_update_puppies();
#endif

    marlin_client_init();
    GCodeInfo::getInstance().Init(gui_media_LFN, gui_media_SFN_path);

    DialogHandler::Access(); // to create class NOW, not at first call of one of callback
    marlin_client_set_fsm_cb(DialogHandler::command_c_compatible);
    marlin_client_set_message_cb(MsgCircleBuffer_cb);
    marlin_client_set_warning_cb(Warning_cb);
    marlin_client_set_startup_cb(Startup_cb);

    Sound_Play(eSOUND_TYPE::Start);

    marlin_client_set_event_notify(marlin_server::EVENT_MSK_DEF, client_gui_refresh);

    GUIStartupProgress progr = { 100, std::nullopt };
    event_conversion_union un;
    un.pGUIStartupProgress = &progr;
    Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);

#if HAS_LEDS && !HAS_SIDE_LEDS()
    // we need to step the animator, to move the started animation to current to let it run for one cycle
    auto guard = leds::start_animation(PrinterState::PowerUp, 10);
    Animator_LCD_leds().Step();
    guard.Stop();
#endif

#if HAS_SIDE_LEDS()
    leds::side_strip_control.ActivityPing();
#endif

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
