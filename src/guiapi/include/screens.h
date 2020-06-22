// pointers to all screens
#pragma once
#include "screen.h" //screen_t

extern screen_t *const get_scr_splash();
extern screen_t *const get_scr_watchdog();
extern screen_t *const get_scr_test();
extern screen_t *const get_scr_test_gui();
extern screen_t *const get_scr_test_term();
extern screen_t *const get_scr_test_msgbox();
extern screen_t *const get_scr_test_graph();
extern screen_t *const get_scr_test_temperature();
extern screen_t *const get_scr_home();
extern screen_t *const get_scr_filebrowser();
extern screen_t *const get_scr_printing();
extern screen_t *const get_scr_printing_serial();
extern screen_t *const get_scr_menu_preheat();
extern screen_t *const get_scr_menu_filament();
extern screen_t *const get_scr_preheating();
extern screen_t *const get_scr_menu_calibration();
extern screen_t *const get_scr_menu_settings();
extern screen_t *const get_scr_menu_temperature();
extern screen_t *const get_scr_menu_move();
extern screen_t *const get_scr_menu_info();
extern screen_t *const get_scr_menu_tune();
extern screen_t *const get_scr_menu_service();
extern screen_t *const get_scr_sysinfo();
extern screen_t *const get_scr_version_info();
extern screen_t *const get_scr_qr_info();
extern screen_t *const get_scr_qr_error();
extern screen_t *const get_scr_test_disp_mem();
extern screen_t *const get_scr_messages();
extern screen_t *const get_scr_mesh_bed_lv();
extern screen_t *const get_scr_wizard();
extern screen_t *const get_scr_print_preview();
extern screen_t *const get_scr_lan_settings();
extern screen_t *const get_scr_marlin();
extern screen_t *const get_scr_menu_fw_update();
extern screen_t *const get_scr_print_preview();
#ifdef PIDCALIBRATION
extern screen_t *const get_scr_PID();
#endif //PIDCALIBRATION
