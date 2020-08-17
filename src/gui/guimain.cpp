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
#ifdef _DEBUG
    #include "window_dlg_popup.hpp"
#endif //_DEBUG
#include "window_dlg_preheat.hpp"
#include "screen_print_preview.hpp"
#include "screen_watchdog.hpp"

#include "DialogHandler.hpp"
#include "sound.hpp"
#include "i18n.h"

extern int HAL_IWDG_Reset;

int guimain_spi_test = 0;

#include "gpio.h"
#include "st7789v.hpp"
#include "jogwheel.h"
#include "hwio.h"
#include "diag.h"
#include "sys.h"
#include "dbg.h"
#include "wdt.h"

const st7789v_config_t st7789v_cfg = {
    &hspi2,             // spi handle pointer
    ST7789V_PIN_CS,     // CS pin
    ST7789V_PIN_RS,     // RS pin
    ST7789V_PIN_RST,    // RST pin
    ST7789V_FLG_DMA,    // flags (DMA, MISO)
    ST7789V_DEF_COLMOD, // interface pixel format (5-6-5, hi-color)
    ST7789V_DEF_MADCTL, // memory data access control (no mirror XY)
};

const jogwheel_config_t jogwheel_cfg = {
    JOGWHEEL_PIN_EN1, // encoder phase1
    JOGWHEEL_PIN_EN2, // encoder phase2
    JOGWHEEL_PIN_ENC, // button
    JOGWHEEL_DEF_FLG, // flags
};

marlin_vars_t *gui_marlin_vars = 0;
int8_t menu_timeout_enabled = 1; // Default: enabled

void update_firmware_screen(void);

static void _gui_loop_cb() {
    marlin_client_loop();
}

char gui_media_LFN[FILE_NAME_MAX_LEN + 1];
char gui_media_SFN_path[FILE_PATH_MAX_LEN + 1];

extern "C" void gui_run(void) {
    if (diag_fastboot)
        return;

    st7789v_config = st7789v_cfg;
    jogwheel_config = jogwheel_cfg;
    gui_init();

    // select jogwheel type by measured 'reset delay'
    // original displays with 15 position encoder returns values 1-2 (short delay - no capacitor)
    // new displays with MK3 encoder returns values around 16000 (long delay - 100nF capacitor)
    if (st7789v_reset_delay > 1000) // threshold value is 1000
        jogwheel_config.flg = JOGWHEEL_FLG_INV_DIR | JOGWHEEL_FLG_INV_ENC;
    //_dbg("delay=%u", st7789v_reset_delay);

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

    Sound_Play(eSOUND_TYPE_Start);

    ScreenFactory::Creator screen_initializer[] {
#ifndef _DEBUG
        HAL_IWDG_Reset ? ScreenFactory::Screen<screen_watchdog_data_t> : nullptr, // wdt
#endif
        ScreenFactory::Screen<screen_splash_data_t>, // splash
        ScreenFactory::Screen<screen_home_data_t>    // home
    };

    //Screens::Init(ScreenFactory::Screen<screen_splash_data_t>);
    Screens::Init(screen_initializer, screen_initializer + (sizeof(screen_initializer) / sizeof(screen_initializer[0])));

    //set loop callback (will be called every time inside gui_loop)
    gui_loop_cb = _gui_loop_cb;
    //int8_t gui_timeout_id;
    while (1) {
        Screens::Access()->Loop();
        // show warning dialog on safety timer expiration
        if (marlin_event_clr(MARLIN_EVT_SafetyTimerExpired)) {
            MsgBoxInfo(_("Heating disabled due to 30 minutes of inactivity."), Responses_Ok);
        }
        gui_loop();
        /*if (marlin_message_received()) {
            screen_t *curr = screen_get_curr();
            if (curr == get_scr_printing()) {
                screen_dispatch_event(NULL, WINDOW_EVENT_MESSAGE, 0);
            }
        }
        if (menu_timeout_enabled) {
            gui_timeout_id = gui_get_menu_timeout_id();
            if (gui_timer_expired(gui_timeout_id) == 1) {
                screen_close_multiple(scrn_close_on_timeout);
                gui_timer_delete(gui_timeout_id);
            }
        }*/
    }
}

void update_firmware_screen(void) {
    font_t *font = resource_font(IDR_FNT_SPECIAL);
    font_t *font1 = resource_font(IDR_FNT_NORMAL);
    display::Clear(COLOR_BLACK);
    render_icon_align(Rect16(70, 20, 100, 100), IDR_PNG_icon_pepa, COLOR_BLACK, RENDER_FLG(ALIGN_CENTER, 0));
    display::DrawText(Rect16(10, 115, 240, 60), _("Hi, this is your\nOriginal Prusa MINI."), font, COLOR_BLACK, COLOR_WHITE);
    display::DrawText(Rect16(10, 160, 240, 80), _("Please insert the USB\ndrive that came with\nyour MINI and reset\nthe printer to flash\nthe firmware"), font, COLOR_BLACK, COLOR_WHITE);
    render_text_align(Rect16(5, 250, 230, 40), _("RESET PRINTER"), font1, COLOR_ORANGE, COLOR_WHITE, { 2, 6, 2, 2 }, ALIGN_CENTER);
    while (1) {
        if (jogwheel_button_down > 50)
            sys_reset();
        osDelay(1);
        wdt_iwdg_refresh();
    }
}
