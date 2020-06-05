#include "MItem_menus.hpp"
#include "screens.h"

/*****************************************************************************/
//MI_VERSION_INFO
MI_VERSION_INFO::MI_VERSION_INFO()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_VERSION_INFO::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_version_info()->id);
}

/*****************************************************************************/
//MI_FILAMENT
MI_FILAMENT::MI_FILAMENT()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FILAMENT::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_menu_filament()->id);
}

/*****************************************************************************/
//MI_SYS_INFO
MI_SYS_INFO::MI_SYS_INFO()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SYS_INFO::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_sysinfo()->id);
}

/*****************************************************************************/
//MI_STATISTIC_disabled
MI_STATISTIC_disabled::MI_STATISTIC_disabled()
    : WI_LABEL_t(label, 0, false, false) {
}

/*****************************************************************************/
//MI_FAIL_STAT_disabled
MI_FAIL_STAT_disabled::MI_FAIL_STAT_disabled()
    : WI_LABEL_t(label, 0, false, false) {
}

/*****************************************************************************/
//MI_SUPPORT_disabled
MI_SUPPORT_disabled::MI_SUPPORT_disabled()
    : WI_LABEL_t(label, 0, false, false) {
}

/*****************************************************************************/
//MI_QR_test
MI_QR_test::MI_QR_test()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_QR_test::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_qr_error()->id);
}

/*****************************************************************************/
//MI_QR_info
MI_QR_info::MI_QR_info()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_QR_info::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_qr_info()->id);
}

/*****************************************************************************/
//MI_TEMPERATURE
MI_TEMPERATURE::MI_TEMPERATURE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_TEMPERATURE::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_menu_temperature()->id);
}

/*****************************************************************************/
//MI_MOVE_AXIS
MI_MOVE_AXIS::MI_MOVE_AXIS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_MOVE_AXIS::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_menu_move()->id);
}

/*****************************************************************************/
//MI_SERVICE
MI_SERVICE::MI_SERVICE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SERVICE::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_menu_service()->id);
}

/*****************************************************************************/
//MI_TEST
MI_TEST::MI_TEST()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_TEST::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_test()->id);
}

/*****************************************************************************/
//MI_FW_UPDATE
MI_FW_UPDATE::MI_FW_UPDATE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FW_UPDATE::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_menu_fw_update()->id);
}

/*****************************************************************************/
//MI_LAN_SETTINGS
MI_LAN_SETTINGS::MI_LAN_SETTINGS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_LAN_SETTINGS::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_lan_settings()->id);
}

/*****************************************************************************/
//MI_MESSAGES
MI_MESSAGES::MI_MESSAGES()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_MESSAGES::click(Iwindow_menu_t & /*window_menu*/) {
    screen_open(get_scr_messages()->id);
}
