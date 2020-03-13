//guimain.c

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "gui.h"
#include "config.h"
#include "marlin_client.h"

#include "window_logo.h"

#ifdef LCDSIM
    #include "window_lcdsim.h"
#else //LCDSIM
    #include "window_file_list.h"
    #include "window_header.h"
    #include "window_temp_graph.h"
    #include "window_dlg_statemachine.h"
    #include "window_dlg_wait.h"
    #ifdef _DEBUG
        #include "window_dlg_popup.h"
    #endif //_DEBUG
    #include "window_dlg_preheat.h"
    #include "window_dlg_change.h"
    #include "screen_print_preview.h"
#endif //LCDSIM

#include "screen_lan_settings.h"
#include "screen_menu_fw_update.h"

extern screen_t *pscreen_splash;
extern screen_t *pscreen_watchdog;

#ifdef LCDSIM
extern screen_t *pscreen_marlin;
#else //LCDSIM
extern screen_t *pscreen_test;
extern screen_t *pscreen_test_gui;
extern screen_t *pscreen_test_term;
extern screen_t *pscreen_test_msgbox;
extern screen_t *pscreen_test_graph;
extern screen_t *pscreen_test_temperature;
extern screen_t *pscreen_home;
extern screen_t *pscreen_filebrowser;
extern screen_t *pscreen_printing;
extern screen_t *pscreen_printing_serial;
extern screen_t *pscreen_menu_preheat;
extern screen_t *pscreen_menu_filament;
extern screen_t *pscreen_preheating;
extern screen_t *pscreen_menu_calibration;
extern screen_t *pscreen_menu_settings;
extern screen_t *pscreen_menu_temperature;
extern screen_t *pscreen_menu_move;
extern screen_t *pscreen_menu_info;
extern screen_t *pscreen_menu_tune;
extern screen_t *pscreen_menu_service;
extern screen_t *pscreen_sysinfo;
extern screen_t *pscreen_version_info;
extern screen_t *pscreen_qr_info;
extern screen_t *pscreen_qr_error;
extern screen_t *pscreen_test_disp_mem;
extern screen_t *pscreen_messages;
    #ifdef PIDCALIBRATION
extern screen_t *pscreen_PID;
    #endif //PIDCALIBRATION
extern screen_t *pscreen_mesh_bed_lv;
extern screen_t *pscreen_wizard;
#endif     // LCDSIM

extern int HAL_IWDG_Reset;

#ifndef _DEBUG
extern IWDG_HandleTypeDef hiwdg; //watchdog handle
#endif                           //_DEBUG

int guimain_spi_test = 0;

#include "gpio.h"
#include "st7789v.h"
#include "jogwheel.h"
#include "hwio.h"
#include "diag.h"
#include "sys.h"
#include "dbg.h"
#include "marlin_host.h"

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
int gui_marlin_client_id = -1;
int8_t menu_timeout_enabled = 1; // Default: enabled

extern screen_t screen_home;
extern screen_t screen_printing;

extern screen_t screen_printing_serial;
extern screen_t screen_menu_tune;
extern screen_t screen_wizard;
extern screen_t screen_print_preview;
extern screen_t screen_PID;

static screen_t *const timeout_blacklist[] = {
    &screen_home,
    &screen_printing,
    &screen_menu_tune,
    &screen_wizard,
    &screen_print_preview
#ifdef PIDCALIBRATION
    ,
    &screen_PID
#endif //PIDCALIBRATION
};

static screen_t *const m876_blacklist[] = {
    &screen_printing_serial,
    &screen_home
#ifdef PIDCALIBRATION
    ,
    &screen_PID
#endif //PIDCALIBRATION
};

void update_firmware_screen(void);

static void _gui_loop_cb() {
    static uint8_t m600_lock = 0;

    if (!m600_lock) {
        m600_lock = 1;
        if (marlin_event_clr(MARLIN_EVT_CommandBegin)) {
            if (marlin_command() == MARLIN_CMD_M600) {
                _dbg("M600 start");
                gui_dlg_change();
                _dbg("M600 end");
            }
        }
        m600_lock = 0;
    }

    marlin_client_loop();
}

static void dialog_open_cb(dialog_t dialog) {
    if (gui_get_nesting() > 1)
        return; //todo notify octoprint
    if (dialog == DLG_serial_printing) {
        screen_unloop(m876_blacklist, sizeof(m876_blacklist) / sizeof(m876_blacklist[0]));

        if (screen_get_curr() != pscreen_printing_serial)
            screen_open(pscreen_printing_serial->id);
    }
}

static void dialog_close_cb(dialog_t dialog) {
    if (gui_get_nesting() > 1)
        return; //todo notify octoprint
    if (dialog == DLG_serial_printing) {
        if (screen_get_curr() == pscreen_printing_serial)
            screen_close();
    }
}

void gui_run(void) {
    if (diag_fastboot)
        return;

    st7789v_config = st7789v_cfg;
    jogwheel_config = jogwheel_cfg;
    gui_init();

    // select jogwheel type by meassured 'reset delay'
    // original displays with 15 position encoder returns values 1-2 (short delay - no capacitor)
    // new displays with MK3 encoder returns values around 16000 (long delay - 100nF capacitor)
    if (st7789v_reset_delay > 1000) // threshold value is 1000
        jogwheel_config.flg = JOGWHEEL_FLG_INV_DIR | JOGWHEEL_FLG_INV_ENC;
    //_dbg("delay=%u", st7789v_reset_delay);

    gui_defaults.font = resource_font(IDR_FNT_NORMAL);
    gui_defaults.font_big = resource_font(IDR_FNT_BIG);
    window_msgbox_id_icon[0] = 0; //IDR_PNG_icon_pepa;
    window_msgbox_id_icon[1] = IDR_PNG_header_icon_error;
    window_msgbox_id_icon[2] = IDR_PNG_header_icon_question;
    window_msgbox_id_icon[3] = IDR_PNG_header_icon_warning;
    window_msgbox_id_icon[4] = IDR_PNG_header_icon_info;

    if (!sys_fw_is_valid())
        update_firmware_screen();

    gui_marlin_vars = marlin_client_init();
    gui_marlin_client_id = marlin_client_id();
    marlin_client_set_dialog_open_cb(dialog_open_cb);
    marlin_client_set_dialog_close_cb(dialog_close_cb);
    hwio_beeper_tone2(440.0, 100, 0.0125); //start beep

    screen_register(pscreen_splash);
    screen_register(pscreen_watchdog);

    WINDOW_CLS_LOGO = window_register_class((window_class_t *)&window_class_logo);
#ifdef LCDSIM
    WINDOW_CLS_LCDSIM = window_register_class((window_class_t *)&window_class_lcdsim);
    screen_register(pscreen_marlin);
#else //LCDSIM
    WINDOW_CLS_FILE_LIST = window_register_class((window_class_t *)&window_class_file_list);
    WINDOW_CLS_HEADER = window_register_class((window_class_t *)&window_class_header);
    WINDOW_CLS_TEMP_GRAPH = window_register_class((window_class_t *)&window_class_temp_graph);
    WINDOW_CLS_DLG_LOADUNLOAD = window_register_class((window_class_t *)&window_class_dlg_statemachine);
    WINDOW_CLS_DLG_WAIT = window_register_class((window_class_t *)&window_class_dlg_wait);
    #ifdef _DEBUG
    WINDOW_CLS_DLG_POPUP = window_register_class((window_class_t *)&window_class_dlg_popup);
    #endif //_DEBUG
    WINDOW_CLS_DLG_PREHEAT = window_register_class((window_class_t *)&window_class_dlg_preheat);
    screen_register(pscreen_test);
    screen_register(pscreen_test_gui);
    screen_register(pscreen_test_term);
    screen_register(pscreen_test_msgbox);
    screen_register(pscreen_test_graph);
    screen_register(pscreen_test_temperature);
    screen_register(pscreen_home);
    screen_register(pscreen_filebrowser);
    screen_register(pscreen_printing);
    screen_register(pscreen_printing_serial);
    screen_register(pscreen_menu_preheat);
    screen_register(pscreen_menu_filament);
    screen_register(pscreen_menu_calibration);
    screen_register(pscreen_menu_settings);
    screen_register(pscreen_menu_temperature);
    screen_register(pscreen_menu_move);
    screen_register(pscreen_menu_info);
    screen_register(pscreen_menu_tune);
    screen_register(pscreen_menu_service);
    screen_register(pscreen_sysinfo);
    screen_register(pscreen_version_info);
    screen_register(pscreen_qr_info);
    screen_register(pscreen_qr_error);
    screen_register(pscreen_test_disp_mem);
    screen_register(pscreen_messages);
    #ifdef PIDCALIBRATION
    screen_register(pscreen_PID);
    #endif //PIDCALIBRATION
    screen_register(pscreen_mesh_bed_lv);
    screen_register(pscreen_wizard);
    screen_register(pscreen_print_preview);
    screen_register(pscreen_lan_settings);
    screen_register(pscreen_menu_fw_update);
#endif     // LCDSIM

#ifndef _DEBUG
    if (HAL_IWDG_Reset) {
        screen_stack_push(pscreen_splash->id);
        screen_open(pscreen_watchdog->id);
    } else
#endif // _DEBUG
        screen_open(pscreen_splash->id);

    //set loop callback (will be called every time inside gui_loop)
    gui_loop_cb = _gui_loop_cb;
    int8_t gui_timeout_id;
    while (1) {
        float vol = 0.01F;
        //simple jogwheel acoustic feedback
        if ((jogwheel_changed & 1) && jogwheel_button_down)       //button changed and pressed
            hwio_beeper_tone2(200.0, 50, (double)(vol * 0.125F)); //beep
        else if (jogwheel_changed & 2)                            // encoder changed
            hwio_beeper_tone2(50.0, 25, (double)(vol * 0.125F));  //short click
        // show warning dialog on safety timer expiration
        if (marlin_event_clr(MARLIN_EVT_SafetyTimerExpired)) {
            gui_msgbox("Heating disabled due to 30 minutes of inactivity.", MSGBOX_BTN_OK | MSGBOX_ICO_WARNING);
        }
        gui_loop();
#ifndef LCDSIM
        if (marlin_message_received()) {
            screen_t *curr = screen_get_curr();
            if (curr == pscreen_printing) {
                screen_dispatch_event(NULL, WINDOW_EVENT_MESSAGE, 0);
            }
        }
        if (menu_timeout_enabled) {
            gui_timeout_id = gui_get_menu_timeout_id();
            if (gui_timer_expired(gui_timeout_id) == 1) {
                screen_unloop(timeout_blacklist, sizeof(timeout_blacklist) / sizeof(timeout_blacklist[0]));
                gui_timer_delete(gui_timeout_id);
            }
        }
#endif //LCDSIM
    }
}

void update_firmware_screen(void) {
    font_t *font = resource_font(IDR_FNT_SPECIAL);
    font_t *font1 = resource_font(IDR_FNT_NORMAL);
    display->fill_rect(rect_ui16(0, 0, 240, 320), COLOR_BLACK);
    render_icon_align(rect_ui16(70, 20, 100, 100), IDR_PNG_icon_pepa, COLOR_BLACK, RENDER_FLG(ALIGN_CENTER, 0));
    display->draw_text(rect_ui16(10, 115, 240, 60), "Hi, this is your\nOriginal Prusa MINI.", font, COLOR_BLACK, COLOR_WHITE);
    display->draw_text(rect_ui16(10, 160, 240, 80), "Please insert the USB\ndrive that came with\nyour MINI and reset\nthe printer to flash\nthe firmware", font, COLOR_BLACK, COLOR_WHITE);
    render_text_align(rect_ui16(5, 250, 230, 40), "RESET PRINTER", font1, COLOR_ORANGE, COLOR_WHITE, padding_ui8(2, 6, 2, 2), ALIGN_CENTER);
    while (1) {
        if (jogwheel_button_down > 50)
            sys_reset();
        osDelay(1);
#ifndef _DEBUG
        HAL_IWDG_Refresh(&hiwdg); //watchdog reset
#endif                            //_DEBUG
    }
}
