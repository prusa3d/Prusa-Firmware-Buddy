// pointers to all screens
#pragma once
#include "screen.h" //screen_t

//extern screen_t *const get_scr_splash();
extern screen_t *const get_scr_watchdog();
extern screen_t *const get_scr_test();
extern screen_t *const get_scr_printing_serial();
extern screen_t *const get_scr_sysinfo();
extern screen_t *const get_scr_version_info();
extern screen_t *const get_scr_qr_info();
extern screen_t *const get_scr_messages();
extern screen_t *const get_scr_wizard();
extern screen_t *const get_scr_print_preview();
extern screen_t *const get_scr_lan_settings();
extern screen_t *const get_scr_marlin();
extern screen_t *const get_scr_menu_fw_update();
extern screen_t *const get_scr_print_preview();
extern screen_t *const get_scr_menu_languages();
extern screen_t *const get_scr_menu_languages_noret();
#ifdef PIDCALIBRATION
extern screen_t *const get_scr_PID();
#endif //PIDCALIBRATION
