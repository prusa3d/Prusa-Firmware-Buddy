// pointers to all screens
#pragma once
#include "screen.h" //screen_t
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern screen_t *pscreen_splash;
extern screen_t *pscreen_watchdog;

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
extern screen_t *const pscreen_print_preview;
extern screen_t *const pscreen_lan_settings;
extern screen_t *pscreen_marlin;
extern screen_t *const pscreen_menu_fw_update;

#ifdef __cplusplus
}
#endif //__cplusplus
