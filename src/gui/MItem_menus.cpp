#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "ScreenHandler.hpp"
#include "screen_sysinf.hpp"
#include "screen_qr_error.hpp"
#include "screen_messages.hpp"
#include "marlin_client.h"
#include "translation_provider_FILE.hpp"
#include "filament_sensor_api.hpp"
#include "translator.hpp"
#include "SteelSheets.hpp"
#include "png_resources.hpp"
#include "screen_menu_filament.hpp"
#include "screen_menu_temperature.hpp"
#include "screen_menu_move.hpp"
#include "screen_menu_sensor_info.hpp"

/*****************************************************************************/
//MI_VERSION_INFO
MI_VERSION_INFO::MI_VERSION_INFO()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_VERSION_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuVersionInfo);
}

/*****************************************************************************/
//MI_SENSOR_INFO
MI_SENSOR_INFO::MI_SENSOR_INFO()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SENSOR_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuSensorInfo>);
}

/*****************************************************************************/
MI_ODOMETER::MI_ODOMETER()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ODOMETER::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuOdometer);
}

/*****************************************************************************/
//MI_FILAMENT
MI_FILAMENT::MI_FILAMENT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilament>);
}

/*****************************************************************************/
//MI_SYS_INFO
MI_SYS_INFO::MI_SYS_INFO()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SYS_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_sysinfo_data_t>);
}

/*****************************************************************************/
//MI_FAIL_STAT_disabled
MI_FAIL_STAT_disabled::MI_FAIL_STAT_disabled()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

/*****************************************************************************/
//MI_SUPPORT_disabled
MI_SUPPORT_disabled::MI_SUPPORT_disabled()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

/*****************************************************************************/
//MI_TEMPERATURE
MI_TEMPERATURE::MI_TEMPERATURE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEMPERATURE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTemperature>);
}

/*****************************************************************************/
//MI_MOVE_AXIS
MI_MOVE_AXIS::MI_MOVE_AXIS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MOVE_AXIS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMove>);
}

/*****************************************************************************/
//MI_SERVICE
MI_SERVICE::MI_SERVICE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SERVICE::click(IWindowMenu & /*window_menu*/) {
    //screen_open(get_scr_menu_service()->id);
}

/*****************************************************************************/
//MI_TEST
MI_TEST::MI_TEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuTest);
}

/*****************************************************************************/
//MI_FW_UPDATE
MI_FW_UPDATE::MI_FW_UPDATE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FW_UPDATE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuFwUpdate);
}

/*****************************************************************************/
//MI_ETH_SETTINGS
MI_ETH_SETTINGS::MI_ETH_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    SetIconId(&png::lan_16x16);
}

void MI_ETH_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuEthernetSettings);
}

/*****************************************************************************/
//MI_WIFI_SETTINGS
MI_WIFI_SETTINGS::MI_WIFI_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    SetIconId(&png::wifi_16x16);
}

void MI_WIFI_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuWifiSettings);
}

/*****************************************************************************/
//MI_MESSAGES
MI_MESSAGES::MI_MESSAGES()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MESSAGES::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_messages_data_t>);
}

/*****************************************************************************/
//MI_LANGUAGE
MI_LANGUAGE::MI_LANGUAGE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_LANGUAGE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuLanguages);
}

/*****************************************************************************/
//MI_HW_SETUP
MI_HW_SETUP::MI_HW_SETUP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_HW_SETUP::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuHwSetup);
}

/*****************************************************************************/
//MI_CURRENT_PROFILE
MI_CURRENT_PROFILE::MI_CURRENT_PROFILE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, SteelSheets::NumOfCalibrated() > 1 ? is_hidden_t::no : is_hidden_t::yes) {
    if (SteelSheets::NumOfCalibrated() > 1) {
        UpdateLabel();
    }
}

void MI_CURRENT_PROFILE::click(IWindowMenu & /*window_menu*/) {
    SteelSheets::NextSheet();
    UpdateLabel();
}

void MI_CURRENT_PROFILE::UpdateLabel() {
    name[0] = '[';
    uint32_t cnt = SteelSheets::ActiveSheetName(name + 1, MAX_SHEET_NAME_LENGTH);
    name[cnt + 1] = ']';
    name[cnt + 2] = 0;
    // string_view_utf8::MakeRAM is safe. "name" is member var, exists until MI_CURRENT_PROFILE is destroyed
    SetLabel(string_view_utf8::MakeRAM((const uint8_t *)name));
}

/*****************************************************************************/
//MI_EEPROM
MI_EEPROM::MI_EEPROM()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EEPROM::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenEeprom);
}

/*****************************************************************************/
//MI_DEVHASH_IN_QR
MI_DEVHASH_IN_QR::MI_DEVHASH_IN_QR()
    : WI_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_DEVHASH_IN_QR), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_DEVHASH_IN_QR::OnChange(size_t old_index) {
    eeprom_set_bool(EEVAR_DEVHASH_IN_QR, !old_index);
}

/*****************************************************************************/
//MI_FOOTER_SETTINGS
MI_FOOTER_SETTINGS::MI_FOOTER_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FOOTER_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuFooterSettings);
}

/*****************************************************************************/
//MI_FOOTER_SETTINGS_ADV
MI_FOOTER_SETTINGS_ADV::MI_FOOTER_SETTINGS_ADV()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FOOTER_SETTINGS_ADV::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuFooterSettingsAdv);
}

/*****************************************************************************/
//MI_LANGUAGUE_USB
MI_LANGUAGUE_USB::MI_LANGUAGUE_USB()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_LANGUAGUE_USB::click(IWindowMenu &windowMenu) {
    if (fileProviderUSB.EnsureFile())
        Translations::Instance().RegisterProvider(Translations::MakeLangCode("ts"), &fileProviderUSB);
}

/*****************************************************************************/
//MI_LOAD_LANG
MI_LOAD_LANG::MI_LOAD_LANG()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_LOAD_LANG::click(IWindowMenu &windowMenu) {
    const uint8_t buffLen = 16;

    uint8_t buff[buffLen];

    FILE *srcDir = fopen("/usb/lang/ts.mo", "rb");
    FILE *dstDir = fopen("/internal/ts.mo", "wb");
    //copy languague from usb to xflash
    if (dstDir && srcDir) {
        for (size_t readBytes = fread(buff, 1, buffLen, srcDir); readBytes != 0; readBytes = fread(buff, 1, buffLen, srcDir)) {
            fwrite(buff, 1, readBytes, dstDir);
        }
    }
    fclose(dstDir);
    fclose(srcDir);
}
MI_LANGUAGUE_XFLASH::MI_LANGUAGUE_XFLASH()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_LANGUAGUE_XFLASH::click(IWindowMenu &windowMenu) {
    if (fileProviderInternal.EnsureFile())
        Translations::Instance().RegisterProvider(Translations::MakeLangCode("ts"), &fileProviderInternal);
}

/*****************************************************************************/
//MI_PRUSALINK
MI_PRUSALINK::MI_PRUSALINK()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_PRUSALINK::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenPrusaLink);
}

/*****************************************************************************/
//MI_PRUSALINK
MI_PRUSA_CONNECT::MI_PRUSA_CONNECT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_PRUSA_CONNECT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuConnect);
}

/**********************************************************************************************/
// MI_NETWORK

MI_NETWORK::MI_NETWORK()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_NETWORK::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuNetwork);
}

//MI_EXPERIMENTAL_SETTINGS
MI_EXPERIMENTAL_SETTINGS::MI_EXPERIMENTAL_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EXPERIMENTAL_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuExperimentalSettings);
}

/**********************************************************************************************/
// MI_EEPROM_DIAGNOSTICS
MI_EEPROM_DIAGNOSTICS::MI_EEPROM_DIAGNOSTICS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_EEPROM_DIAGNOSTICS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(GetScreenMenuEepromDiagnostics);
}

/**********************************************************************************************/
// MI_USB_MSC_ENABLE
MI_USB_MSC_ENABLE::MI_USB_MSC_ENABLE()
    : WI_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_USB_MSC_ENABLED), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_USB_MSC_ENABLE::OnChange(size_t old_index) {
    eeprom_set_bool(EEVAR_USB_MSC_ENABLED, !old_index);
}
