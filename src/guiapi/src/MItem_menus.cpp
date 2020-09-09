#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "ScreenHandler.hpp"
#include "screen_sysinf.hpp"
#include "screen_qr_error.hpp"
#include "screen_test.hpp"
#include "screen_messages.hpp"

/*****************************************************************************/
//MI_VERSION_INFO
MI_VERSION_INFO::MI_VERSION_INFO()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_VERSION_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuVersionInfo);
}

/*****************************************************************************/
//MI_FILAMENT
MI_FILAMENT::MI_FILAMENT()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuFilament);
}

/*****************************************************************************/
//MI_SYS_INFO
MI_SYS_INFO::MI_SYS_INFO()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SYS_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_sysinfo_data_t>);
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
//MI_TEMPERATURE
MI_TEMPERATURE::MI_TEMPERATURE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_TEMPERATURE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuTemperature);
}

/*****************************************************************************/
//MI_MOVE_AXIS
MI_MOVE_AXIS::MI_MOVE_AXIS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_MOVE_AXIS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuMove);
}

/*****************************************************************************/
//MI_SERVICE
MI_SERVICE::MI_SERVICE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_SERVICE::click(IWindowMenu & /*window_menu*/) {
    //screen_open(get_scr_menu_service()->id);
}

/*****************************************************************************/
//MI_TEST
MI_TEST::MI_TEST()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_TEST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_test_data_t>);
}

/*****************************************************************************/
//MI_FW_UPDATE
MI_FW_UPDATE::MI_FW_UPDATE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_FW_UPDATE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuFwUpdate);
}

/*****************************************************************************/
//MI_LAN_SETTINGS
MI_LAN_SETTINGS::MI_LAN_SETTINGS()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_LAN_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuLanSettings);
}

/*****************************************************************************/
//MI_MESSAGES
MI_MESSAGES::MI_MESSAGES()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_MESSAGES::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_messages_data_t>);
}

/*****************************************************************************/
//MI_LANGUAGE
MI_LANGUAGE::MI_LANGUAGE()
    : WI_LABEL_t(label, 0, true, false) {
}

void MI_LANGUAGE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuLanguages);
}
