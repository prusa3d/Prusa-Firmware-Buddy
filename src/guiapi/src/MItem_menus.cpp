#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "ScreenHandler.hpp"
#include "screen_sysinf.hpp"
#include "screen_qr_error.hpp"
#include "screen_test.hpp"
#include "screen_messages.hpp"
#include "marlin_client.h"

/*****************************************************************************/
//MI_VERSION_INFO
MI_VERSION_INFO::MI_VERSION_INFO()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_VERSION_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuVersionInfo);
}

/*****************************************************************************/
//MI_SENSOR_INFO
MI_SENSOR_INFO::MI_SENSOR_INFO()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SENSOR_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuSensorInfo);
}

/*****************************************************************************/
MI_ODOMETER::MI_ODOMETER()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ODOMETER::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuOdometer);
}

/*****************************************************************************/
//MI_FILAMENT
MI_FILAMENT::MI_FILAMENT()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuFilament);
}

/*****************************************************************************/
//MI_SYS_INFO
MI_SYS_INFO::MI_SYS_INFO()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SYS_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_sysinfo_data_t>);
}

/*****************************************************************************/
//MI_STATISTIC_disabled
MI_STATISTIC_disabled::MI_STATISTIC_disabled()
    : WI_LABEL_t(_(label), 0, is_enabled_t::no, is_hidden_t::no) {
}

/*****************************************************************************/
//MI_FAIL_STAT_disabled
MI_FAIL_STAT_disabled::MI_FAIL_STAT_disabled()
    : WI_LABEL_t(_(label), 0, is_enabled_t::no, is_hidden_t::no) {
}

/*****************************************************************************/
//MI_SUPPORT_disabled
MI_SUPPORT_disabled::MI_SUPPORT_disabled()
    : WI_LABEL_t(_(label), 0, is_enabled_t::no, is_hidden_t::no) {
}

/*****************************************************************************/
//MI_TEMPERATURE
MI_TEMPERATURE::MI_TEMPERATURE()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEMPERATURE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuTemperature);
}

/*****************************************************************************/
//MI_MOVE_AXIS
MI_MOVE_AXIS::MI_MOVE_AXIS()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MOVE_AXIS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuMove);
}

/*****************************************************************************/
//MI_SERVICE
MI_SERVICE::MI_SERVICE()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SERVICE::click(IWindowMenu & /*window_menu*/) {
    //screen_open(get_scr_menu_service()->id);
}

/*****************************************************************************/
//MI_TEST
MI_TEST::MI_TEST()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_test_data_t>);
}

/*****************************************************************************/
//MI_FW_UPDATE
MI_FW_UPDATE::MI_FW_UPDATE()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FW_UPDATE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuFwUpdate);
}

/*****************************************************************************/
//MI_LAN_SETTINGS
MI_LAN_SETTINGS::MI_LAN_SETTINGS()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_LAN_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuLanSettings);
}

/*****************************************************************************/
//MI_MESSAGES
MI_MESSAGES::MI_MESSAGES()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MESSAGES::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_messages_data_t>);
}

/*****************************************************************************/
//MI_LANGUAGE
MI_LANGUAGE::MI_LANGUAGE()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_LANGUAGE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuLanguages);
}

/*****************************************************************************/
//MI_HW_SETUP
MI_HW_SETUP::MI_HW_SETUP()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_HW_SETUP::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuHwSetup);
}

/*****************************************************************************/
//MI_CURRENT_PROFILE
MI_CURRENT_PROFILE::MI_CURRENT_PROFILE()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::yes) {
}

void MI_CURRENT_PROFILE::click(IWindowMenu & /*window_menu*/) {
    sheet_next_calibrated();
    UpdateLabel();
    marlin_settings_load();
}

void MI_CURRENT_PROFILE::UpdateLabel() {
    name[0] = '[';
    uint32_t cnt = sheet_active_name(name + 1, MAX_SHEET_NAME_LENGTH);
    name[cnt + 1] = ']';
    name[cnt + 2] = 0;
    // string_view_utf8::MakeRAM is safe. "name" is member var, exists until MI_CURRENT_PROFILE is destroyed
    SetLabel(string_view_utf8::MakeRAM((const uint8_t *)name));
}

/*****************************************************************************/
//MI_EEPROM
MI_EEPROM::MI_EEPROM()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EEPROM::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenEeprom);
}
