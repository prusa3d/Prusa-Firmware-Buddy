//guimain.cpp

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "gui.hpp"
#include "config.h"
#include "marlin_client.h"

#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "screen_menus.hpp"
#include "window_file_list.hpp"
#include "window_header.hpp"
#include "window_temp_graph.hpp"
#include "window_dlg_wait.hpp"
#include "window_dlg_popup.hpp"
#include "window_dlg_preheat.hpp"
#include "screen_print_preview.hpp"
#include "screen_hardfault.hpp"
#include "screen_temperror.hpp"
#include "screen_watchdog.hpp"
#include "IScreenPrinting.hpp"
#include "DialogHandler.hpp"
#include "sound.hpp"
#include "i18n.h"
#include "eeprom.h"

extern int HAL_IWDG_Reset;

int guimain_spi_test = 0;

#include "gpio.h"
#include "Jogwheel.hpp"
#include "hwio.h"
#include "diag.h"
#include "sys.h"
#include "dbg.h"
#include "wdt.h"
#include "dump.h"

const st7789v_config_t st7789v_cfg = {
    &hspi2,             // spi handle pointer
    ST7789V_FLG_DMA,    // flags (DMA, MISO)
    ST7789V_DEF_COLMOD, // interface pixel format (5-6-5, hi-color)
    ST7789V_DEF_MADCTL, // memory data access control (no mirror XY)
};

marlin_vars_t *gui_marlin_vars = 0;

void update_firmware_screen(void);

static void _gui_loop_cb() {
    marlin_client_loop();
}

char gui_media_LFN[FILE_NAME_MAX_LEN + 1];
char gui_media_SFN_path[FILE_PATH_MAX_LEN + 1];

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

void gui_run(void) {
    if (diag_fastboot)
        return;

    st7789v_config = st7789v_cfg;

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

    if (!sys_fw_is_valid())
        update_firmware_screen();

    gui_marlin_vars = marlin_client_init();
    gui_marlin_vars->media_LFN = gui_media_LFN;
    gui_marlin_vars->media_SFN_path = gui_media_SFN_path;

    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF);

    DialogHandler::Access(); //to create class NOW, not at first call of one of callback
    marlin_client_set_fsm_create_cb(DialogHandler::Open);
    marlin_client_set_fsm_destroy_cb(DialogHandler::Close);
    marlin_client_set_fsm_change_cb(DialogHandler::Change);
    marlin_client_set_message_cb(MsgCircleBuffer_cb);

    Sound_Play(eSOUND_TYPE::Start);

    ScreenFactory::Creator error_screen = nullptr;
    if (!dump_in_xflash_is_displayed()) {
        switch (dump_in_xflash_get_type()) {
        case DUMP_HARDFAULT:
            error_screen = ScreenFactory::Screen<screen_hardfault_data_t>;
            break;
        case DUMP_TEMPERROR:
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

#ifndef _DEBUG
//        HAL_IWDG_Reset ? ScreenFactory::Screen<screen_watchdog_data_t> : nullptr, // wdt
#endif

    ScreenFactory::Creator screen_initializer[] {
        error_screen,
        ScreenFactory::Screen<screen_splash_data_t>, // splash
        ScreenFactory::Screen<screen_home_data_t>    // home
    };

    //Screens::Init(ScreenFactory::Screen<screen_splash_data_t>);
    Screens::Init(screen_initializer, screen_initializer + (sizeof(screen_initializer) / sizeof(screen_initializer[0])));

    //TIMEOUT variable getting value from EEPROM when EEPROM interface is inicialized
    if (variant_get_ui8(eeprom_get_var(EEVAR_MENU_TIMEOUT)) != 0) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }
    //set loop callback (will be called every time inside gui_loop)
    gui_loop_cb = _gui_loop_cb;
    while (1) {
        Screens::Access()->Loop();
        // show warning dialog on safety timer expiration
        if (marlin_event_clr(MARLIN_EVT_SafetyTimerExpired)) {
            MsgBoxInfo(_("Heating disabled due to 30 minutes of inactivity."), Responses_Ok);
        }
        gui_loop();
    }
}

void update_firmware_screen(void) {
    font_t *font = resource_font(IDR_FNT_SPECIAL);
    font_t *font1 = resource_font(IDR_FNT_NORMAL);
    display::Clear(COLOR_BLACK);
    render_icon_align(Rect16(70, 20, 100, 100), IDR_PNG_pepa_64px, COLOR_BLACK, RENDER_FLG(ALIGN_CENTER, 0));
    display::DrawText(Rect16(10, 115, 240, 60), _("Hi, this is your\nOriginal Prusa MINI."), font, COLOR_BLACK, COLOR_WHITE);
    display::DrawText(Rect16(10, 160, 240, 80), _("Please insert the USB\ndrive that came with\nyour MINI and reset\nthe printer to flash\nthe firmware"), font, COLOR_BLACK, COLOR_WHITE);
    render_text_align(Rect16(5, 250, 230, 40), _("RESET PRINTER"), font1, COLOR_ORANGE, COLOR_WHITE, { 2, 6, 2, 2 }, ALIGN_CENTER);
    Jogwheel::BtnState_t btn_ev;
    while (1) {
        if (jogwheel.ConsumeButtonEvent(btn_ev) && btn_ev == Jogwheel::BtnState_t::Held)
            sys_reset();
        osDelay(1);
        wdt_iwdg_refresh();
    }
}
