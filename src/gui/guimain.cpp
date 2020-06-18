//guimain.c

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "gui.h"
#include "config.h"
#include "marlin_client.h"

#include "window_file_list.h"
#include "window_header.h"
#include "window_temp_graph.h"
#include "DialogLoadUnload.h"
#include "window_dlg_wait.h"
#ifdef _DEBUG
    #include "window_dlg_popup.h"
#endif //_DEBUG
#include "window_dlg_preheat.h"
#include "screen_print_preview.h"

#include "screen_lan_settings.h"
#include "screen_menu_fw_update.h"
#include "Dialog_C_wrapper.h"
#include "screens.h"
#include "screen_close_multiple.h"
#include "sound_C_wrapper.h"
#include "../lang/i18n.h"

extern int HAL_IWDG_Reset;

int guimain_spi_test = 0;

#include "gpio.h"
#include "st7789v.h"
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
char gui_media_SFN_path[FILE_PATH_MAX_LEN + 1]; //@@TODO DR - tohle pouzit na ulozeni posledni cesty

extern "C" void gui_run(void) {
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
    gui_marlin_vars->media_LFN = gui_media_LFN;
    gui_marlin_vars->media_SFN_path = gui_media_SFN_path;

    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF);
    register_dialog_callbacks();
    Sound_Play(eSOUND_TYPE_Start);

    screen_register(get_scr_splash());
    screen_register(get_scr_watchdog());

    WINDOW_CLS_FILE_LIST = window_register_class((window_class_t *)&window_class_file_list);
    WINDOW_CLS_HEADER = window_register_class((window_class_t *)&window_class_header);
    WINDOW_CLS_TEMP_GRAPH = window_register_class((window_class_t *)&window_class_temp_graph);
    WINDOW_CLS_DLG_LOADUNLOAD = window_register_class((window_class_t *)&window_class_dlg_statemachine);
    WINDOW_CLS_DLG_WAIT = window_register_class((window_class_t *)&window_class_dlg_wait);
#ifdef _DEBUG
    WINDOW_CLS_DLG_POPUP = window_register_class((window_class_t *)&window_class_dlg_popup);
#endif //_DEBUG
    WINDOW_CLS_DLG_PREHEAT = window_register_class((window_class_t *)&window_class_dlg_preheat);
    screen_register(get_scr_test());
    screen_register(get_scr_test_gui());
    screen_register(get_scr_test_term());
    screen_register(get_scr_test_msgbox());
    screen_register(get_scr_test_graph());
    screen_register(get_scr_test_temperature());
    screen_register(get_scr_home());
    screen_register(get_scr_filebrowser());
    screen_register(get_scr_printing());
    screen_register(get_scr_printing_serial());
    screen_register(get_scr_menu_preheat());
    screen_register(get_scr_menu_filament());
    screen_register(get_scr_menu_calibration());
    screen_register(get_scr_menu_settings());
    screen_register(get_scr_menu_temperature());
    screen_register(get_scr_menu_move());
    screen_register(get_scr_menu_info());
    screen_register(get_scr_menu_tune());
    screen_register(get_scr_menu_service());
    screen_register(get_scr_sysinfo());
    screen_register(get_scr_version_info());
    screen_register(get_scr_qr_info());
    screen_register(get_scr_qr_error());
    screen_register(get_scr_test_disp_mem());
    screen_register(get_scr_messages());
#ifdef PIDCALIBRATION
    screen_register(get_scr_PID());
#endif //PIDCALIBRATION
    screen_register(get_scr_mesh_bed_lv());
    screen_register(get_scr_wizard());
    screen_register(get_scr_print_preview());
    screen_register(get_scr_lan_settings());
    screen_register(get_scr_menu_fw_update());

#ifndef _DEBUG
    if (HAL_IWDG_Reset) {
        screen_stack_push(get_scr_splash()->id);
        screen_open(get_scr_watchdog()->id);
    } else
#endif // _DEBUG
        screen_open(get_scr_splash()->id);

    //set loop callback (will be called every time inside gui_loop)
    gui_loop_cb = _gui_loop_cb;
    int8_t gui_timeout_id;
    while (1) {
        // show warning dialog on safety timer expiration
        if (marlin_event_clr(MARLIN_EVT_SafetyTimerExpired)) {
            gui_msgbox(_("Heating disabled due to 30 minutes of inactivity."), MSGBOX_BTN_OK | MSGBOX_ICO_WARNING);
        }
        gui_loop();
        if (marlin_message_received()) {
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
        }
    }
}

void update_firmware_screen(void) {
    font_t *font = resource_font(IDR_FNT_SPECIAL);
    font_t *font1 = resource_font(IDR_FNT_NORMAL);
    display::Clear(COLOR_BLACK);
    render_icon_align(rect_ui16(70, 20, 100, 100), IDR_PNG_icon_pepa, COLOR_BLACK, RENDER_FLG(ALIGN_CENTER, 0));
    display::DrawText(rect_ui16(10, 115, 240, 60), _("Hi, this is your\nOriginal Prusa MINI."), font, COLOR_BLACK, COLOR_WHITE);
    display::DrawText(rect_ui16(10, 160, 240, 80), _("Please insert the USB\ndrive that came with\nyour MINI and reset\nthe printer to flash\nthe firmware"), font, COLOR_BLACK, COLOR_WHITE);
    render_text_align(rect_ui16(5, 250, 230, 40), _("RESET PRINTER"), font1, COLOR_ORANGE, COLOR_WHITE, padding_ui8(2, 6, 2, 2), ALIGN_CENTER);
    while (1) {
        if (jogwheel_button_down > 50)
            sys_reset();
        osDelay(1);
        wdt_iwdg_refresh();
    }
}
