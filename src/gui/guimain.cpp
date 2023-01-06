#include <stdio.h>
#include "gui_time.hpp"
#include "gui.hpp"
#include "config.h"
#include "marlin_client.h"
#include "display.h"

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
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
#include "eeprom.h"
#include "w25x.h"
#include "gui_fsensor_api.hpp"
#include "tasks.h"
#include "gcode_info.hpp"
#include "version.h"

#include <option/bootloader.h>
#include <option/bootloader_update.h>
#include <option/resources.h>

#if ENABLED(RESOURCES())
    #include "resources/bootstrap.hpp"
    #include "resources/revision_standard.hpp"
#endif
#if BOTH(RESOURCES(), BOOTLOADER())
    #include "bootloader/bootloader.hpp"
#endif
int guimain_spi_test = 0;

#include "gpio.h"
#include "Jogwheel.hpp"
#include "hwio.h"
#include "sys.h"
#include "wdt.h"
#include <crash_dump/dump.h>
#include "gui_media_events.hpp"
#include "main.h"
#include "bsod.h"
#include "log.h"

LOG_COMPONENT_REF(Buddy);
extern void blockISR(); // do not want to include marlin temperature

#ifdef USE_ST7789
const st7789v_config_t st7789v_cfg = {
    &hspi2,             // spi handle pointer
    ST7789V_FLG_DMA,    // flags (DMA, MISO)
    ST7789V_DEF_COLMOD, // interface pixel format (5-6-5, hi-color)
    ST7789V_DEF_MADCTL, // memory data access control (no mirror XY)
};
#endif // USE_ST7789

marlin_vars_t *gui_marlin_vars = 0;

char gui_media_LFN[FILE_NAME_BUFFER_LEN];
char gui_media_SFN_path[FILE_PATH_BUFFER_LEN];

static uint8_t cnt_scan_register_update = 0;
uint8_t data_buff[2] = { 0x00 };

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
    case WarningType::USBFlashDiskError:
        window_dlg_strong_warning_t::ShowUSBFlashDisk();
        break;
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
        unsigned percent = (tick - start) / (3000 / 100); // 3000ms / 100%
        percent = ((percent < 99) ? percent : 99);

        GUIStartupProgress progr = { unsigned(percent), nullptr };
        event_conversion_union un;
        un.pGUIStartupProgress = &progr;
        Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);

        last_tick = tick;
        gui_redraw();
    }
}

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
                    stage_description = "Preparing bootstrap";
                    break;
                case buddy::resources::BootstrapStage::CopyingFiles:
                    stage_description = "Installing files";
                    break;
                default:
                    bsod("unreachable");
                }
                log_info(Buddy, "Bootstrap progress %s (%i %%)", stage_description, percent_done);
                screen_splash_data_t::bootstrap_cb(percent_done, stage_description);
                gui_redraw();
            });
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
}

void gui_error_run(void) {
#ifdef USE_ST7789
    st7789v_config = st7789v_cfg;
#endif

    gui_init();

    ScreenFactory::Creator error_screen = nullptr;
    // If both redscreen and bsod are pending - both are set as displayed, but redscreen is displayed
    if (dump_err_in_xflash_is_valid() && !dump_err_in_xflash_is_displayed()) {
        error_screen = ScreenFactory::Screen<ScreenErrorQR>;
        dump_err_in_xflash_set_displayed();
    }
    if (dump_in_xflash_is_valid() && !dump_in_xflash_is_displayed()) {
        if (error_screen == nullptr) {
            switch (dump_in_xflash_get_type()) {
            case DUMP_HARDFAULT:
                error_screen = ScreenFactory::Screen<screen_hardfault_data_t>;
                break;
#ifndef _DEBUG
            case DUMP_IWDGW:
                error_screen = ScreenFactory::Screen<screen_watchdog_data_t>;
                break;
#endif
            }
        }
        dump_in_xflash_set_displayed();
    }

    screen_node screen_initializer { error_screen };
    Screens::Init(screen_initializer);

    while (true) {
        gui::StartLoop();
        Screens::Access()->Loop();
        gui_bare_loop();
        gui::EndLoop();
    }
}

void gui_run(void) {
#ifdef USE_ST7789
    st7789v_config = st7789v_cfg;
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
    if (eeprom_get_bool(EEVAR_MENU_TIMEOUT)) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }

    Screens::Access()->Loop();

#if ENABLED(RESOURCES())
    finish_update();
#endif
    manufacture_report();
    provide_dependecy(ComponentDependencies::USBSERIAL_READY); // postpone starting Marlin after USBSerial handling in manufacture_report()

    gui_marlin_vars = marlin_client_init();
    gui_marlin_vars->media_LFN = gui_media_LFN;
    gui_marlin_vars->media_SFN_path = gui_media_SFN_path;
    GCodeInfo::getInstance().Init(gui_media_LFN, gui_media_SFN_path);

    DialogHandler::Access(); // to create class NOW, not at first call of one of callback
    marlin_client_set_fsm_cb(DialogHandler::Command);
    marlin_client_set_message_cb(MsgCircleBuffer_cb);
    marlin_client_set_warning_cb(Warning_cb);
    marlin_client_set_startup_cb(Startup_cb);

    Sound_Play(eSOUND_TYPE::Start);

    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF, client_gui_refresh);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF, client_gui_refresh);

    GUIStartupProgress progr = { 100, std::nullopt };
    event_conversion_union un;
    un.pGUIStartupProgress = &progr;
    Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);

    redraw_cmd_t redraw;

    // TODO make some kind of registration
    while (1) {
        gui::StartLoop();
        if (screen_home_data_t::EverBeenOpened()) {
            gui::fsensor::validate_for_cyclical_calls();
        }

        if (++cnt_scan_register_update >= 100) {
            display::ReadMADCTL(data_buff, 1);
            if (data_buff[1] != 0xE0 && data_buff[1] != 0xF0) {
                display::Init();
                Screens::Access()->SetDisplayReinitialized();
            }

            cnt_scan_register_update = 0;
        }

        // I must do it before screen and dialog loops
        // do not use marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE))->print_state, it can make gui freeze in case main thread is unresponsive
        volatile bool print_processor_waiting = marlin_vars()->print_state == mpsWaitGui;

        redraw = DialogHandler::Access().Loop();

        if (redraw == redraw_cmd_t::redraw)
            // all messages received, redraw changes immediately
            gui_redraw();

        // this code handles start of print
        // it must be in main gui loop just before screen handler to ensure no FSM is opened
        // !DialogHandler::Access().IsAnyOpen() - wait until all FSMs are closed (including one click print)
        // one click print is closed automatically from main thread, because it is opened for wrong gcode
        if (print_processor_waiting) {

            // Screens::Access()->Count() == 0      - there are no closed screens under current one == only home screen is opened
            bool can_start_print_at_current_screen = Screens::Access()->Count() == 0 || (Screens::Access()->Count() == 1 && (Screens::Access()->IsScreenOpened<screen_filebrowser_data_t>() || Screens::Access()->IsScreenOpened<screen_printing_data_t>()));
            bool in_preview = Screens::Access()->Count() == 1 && Screens::Access()->IsScreenOpened<ScreenPrintPreview>();

            if ((!DialogHandler::Access().IsAnyOpen()) && can_start_print_at_current_screen) {
                bool have_file_browser = Screens::Access()->IsScreenOnStack<screen_filebrowser_data_t>();
                Screens::Access()->CloseAll(); // set flag to close all screens
                if (have_file_browser)
                    Screens::Access()->Open(ScreenFactory::Screen<screen_filebrowser_data_t>);
                Screens::Access()->Loop(); // close those screens before marlin_gui_ready_to_print

                // notify server, that GUI is ready to print
                marlin_gui_ready_to_print();

                // wait for start of the print - to prevent any unwanted gui action
                while (
                    (marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE))->print_state != mpsIdle) // main thread is processing a print. Here we need to call marlin_update_vars
                    && (!DialogHandler::Access().IsAnyOpen())                                         // wait for print screen to open, any fsm can break waiting (not only open of print screen)
                ) {
                    gui_timers_cycle();   // refresh GUI time
                    marlin_client_loop(); // refresh fsm - required for dialog handler
                    DialogHandler::Access().Loop();
                }
            } else if (!in_preview) {
                marlin_gui_cant_print();
            } // else -> we are in the preview screen. It closes itself from another thread, so we just wait for it to happen.
        }

        Screens::Access()->Loop();
        // Do not redraw if there's an unread FSM message.
        // New screen can be created already but FSM message can change it
        // so it's too soon to draw it.
        if (redraw != redraw_cmd_t::skip)
            gui_loop();
        gui::EndLoop();
    }
}
