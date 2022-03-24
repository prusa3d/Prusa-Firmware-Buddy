//guimain.cpp

#include <stdio.h>
#include "gui_time.hpp"
#include "gui.hpp"
#include "config.h"
#include "marlin_client.h"
#include "display.h"

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "screen_menus.hpp"
#include "window_file_list.hpp"
#include "window_header.hpp"
#include "window_temp_graph.hpp"
#include "window_dlg_wait.hpp"
#include "window_dlg_popup.hpp"
#include "window_dlg_strong_warning.hpp"
#include "window_dlg_preheat.hpp"
#include "screen_print_preview.hpp"
#include "screen_hardfault.hpp"
#include "screen_temperror.hpp"
#include "screen_watchdog.hpp"
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

#ifdef HAS_RESOURCES
    #include "resources/bootstrap.hpp"
#endif

extern int HAL_IWDG_Reset;

int guimain_spi_test = 0;

#include "gpio.h"
#include "Jogwheel.hpp"
#include "hwio.h"
#include "sys.h"
#include "wdt.h"
#include "dump.h"
#include "gui_media_events.hpp"
#include "main.h"
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

#ifdef GUI_JOGWHEEL_SUPPORT
Jogwheel jogwheel;
#endif // GUI_JOGWHEEL_SUPPORT

MsgBuff_t &MsgCircleBuffer() {
    static CircleStringBuffer<MSG_STACK_SIZE, MSG_MAX_LENGTH> ret;
    return ret;
}

void MsgCircleBuffer_cb(const char *txt) {
    MsgCircleBuffer().push_back(txt);
    //cannot open == already openned
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
        unsigned percent = (tick - start) / (3000 / 100); //3000ms / 100%
        percent = ((percent < 99) ? percent : 99);

        GUIStartupProgress progr = { unsigned(percent), nullptr };
        event_conversion_union un;
        un.pGUIStartupProgress = &progr;
        Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);

        last_tick = tick;
        gui_redraw();
    }
}

#ifdef HAS_RESOURCES
static void bootstrap() {
    if (buddy::resources::is_bootstrap_needed()) {
        buddy::resources::bootstrap([](int percent_done, std::optional<const char *> description) {
            if (percent_done) {
                _log_event(LOG_SEVERITY_INFO, log_component_find("Buddy"), "Bootstrap progress changed: %i %%", percent_done);
            }
            if (description.has_value()) {
                _log_event(LOG_SEVERITY_INFO, log_component_find("Buddy"), "Bootstrap description changed: %s", description.value());
            }
            screen_splash_data_t::bootstrap_cb(percent_done, description);
            gui_redraw();
        });
    }
}
#endif

void gui_run(void) {
#ifdef USE_ST7789
    st7789v_config = st7789v_cfg;
#endif

    gui_init();

    // select jogwheel type by measured 'reset delay'
    // original displays with 15 position encoder returns values 1-2 (short delay - no capacitor)
    // new displays with MK3 encoder returns values around 16000 (long delay - 100nF capacitor)
#ifdef GUI_JOGWHEEL_SUPPORT
    #ifdef USE_ST7789
    // run-time jogwheel type detection decides which type of jogwheel device has (each type has different encoder behaviour)
    jogwheel.SetJogwheelType(st7789v_reset_delay);
    #else /* ! USE_ST7789 */
    jogwheel.SetJogwheelType(0);
    #endif
#endif

    GuiDefaults::Font = resource_font(IDR_FNT_NORMAL);
    GuiDefaults::FontBig = resource_font(IDR_FNT_BIG);
    GuiDefaults::FontMenuItems = resource_font(IDR_FNT_NORMAL);
    GuiDefaults::FontMenuSpecial = resource_font(IDR_FNT_SPECIAL);
    GuiDefaults::FooterFont = resource_font(IDR_FNT_SPECIAL);

    gui_marlin_vars = marlin_client_init();
    gui_marlin_vars->media_LFN = gui_media_LFN;
    gui_marlin_vars->media_SFN_path = gui_media_SFN_path;

    DialogHandler::Access(); //to create class NOW, not at first call of one of callback
    marlin_client_set_fsm_cb(DialogHandler::Command);
    marlin_client_set_message_cb(MsgCircleBuffer_cb);
    marlin_client_set_warning_cb(Warning_cb);
    marlin_client_set_startup_cb(Startup_cb);

    Sound_Play(eSOUND_TYPE::Start);

    gui::knob::RegisterHeldLeftAction(TakeAScreenshot);
    gui::knob::RegisterLongPressScreenAction(DialogMoveZ::Show);

    ScreenFactory::Creator error_screen = nullptr;
    if (w25x_init()) {
        if (dump_in_xflash_is_valid() && !dump_in_xflash_is_displayed()) {
            blockISR(); //TODO delete blockISR() on this line to enable start after click
            switch (dump_in_xflash_get_type()) {
            case DUMP_HARDFAULT:
                error_screen = ScreenFactory::Screen<screen_hardfault_data_t>;
                break;
            case DUMP_TEMPERROR:
                //TODO uncomment to enable start after click
                //blockISR();
                error_screen = ScreenFactory::Screen<screen_temperror_data_t>;
                break;
#ifndef _DEBUG
            case DUMP_IWDGW:
                error_screen = ScreenFactory::Screen<screen_watchdog_data_t>;
                break;
#endif
            }
            dump_in_xflash_set_displayed();
        }
    } else {
        //TODO: hardware error
    }

#ifndef _DEBUG
//        HAL_IWDG_Reset ? ScreenFactory::Screen<screen_watchdog_data_t> : nullptr, // wdt
#endif

    screen_node screen_initializer[] {
        error_screen,
        ScreenFactory::Screen<screen_splash_data_t>, // splash
        ScreenFactory::Screen<screen_home_data_t>    // home
    };

    //Screens::Init(ScreenFactory::Screen<screen_splash_data_t>);
    Screens::Init(screen_initializer, screen_initializer + (sizeof(screen_initializer) / sizeof(screen_initializer[0])));

    //TIMEOUT variable getting value from EEPROM when EEPROM interface is initialized
    if (variant8_get_ui8(eeprom_get_var(EEVAR_MENU_TIMEOUT)) != 0) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }

    Screens::Access()->Loop();

#ifdef HAS_RESOURCES
    bootstrap();
#endif

    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF, client_gui_refresh);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF, client_gui_refresh);

    GUIStartupProgress progr = { 100, std::nullopt };
    event_conversion_union un;
    un.pGUIStartupProgress = &progr;
    Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);

    redraw_cmd_t redraw;
    //TODO make some kind of registration
    while (1) {
        gui::StartLoop();
        if (screen_home_data_t::EverBeenOpenned()) {
            gui::fsensor::validate_for_cyclical_calls();
        }
        redraw = DialogHandler::Access().Loop();
        if (redraw == redraw_cmd_t::redraw)
            // all messages received, redraw changes immediately
            gui_redraw();
        Screens::Access()->Loop();
        // Do not redraw if there's an unread FSM message.
        // New screen can be created already but FSM message can change it
        // so it's too soon to draw it.
        if (redraw != redraw_cmd_t::skip)
            gui_loop();
        gui::EndLoop();
    }
}
