#include "MItem_menus.hpp"
#include "ScreenHandler.hpp"
#include <option/buddy_enable_connect.h>
#if BUDDY_ENABLE_CONNECT()
    #include <connect/marlin_printer.hpp>
#endif
#include "screen_sysinf.hpp"
#include "screen_qr_error.hpp"
#include "screen_messages.hpp"
#include "marlin_client.hpp"
#include "menu_spin_config.hpp"
#include "filament_sensors_handler.hpp"
#include "trinamic.h"
#include "../../../lib/Marlin/Marlin/src/module/stepper.h"
#include "Marlin/src/feature/bed_preheat.hpp"
#if ENABLED(PRUSA_TOOLCHANGER)
    #include "../../../lib/Marlin/Marlin/src/module/prusa/toolchanger.h"
    #include "screen_menu_tools.hpp"
    #include <window_tool_action_box.hpp>
#endif
#include <netdev.h>
#include <wui.h>
#include "translator.hpp"
#include "SteelSheets.hpp"
#include <option/has_leds.h>
#if HAS_LEDS()
    #include "led_animations/animator.hpp"
#endif
#include "img_resources.hpp"
#include "power_panic.hpp"
#include "screen_menu_filament.hpp"
#include "screen_menu_temperature.hpp"
#include "screen_menu_move.hpp"
#include "screen_menu_sensor_info.hpp"
#include "screen_menu_odometer.hpp"
#include "screen_menu_version_info.hpp"
#include "screen_menu_metrics.hpp"
#include "screen_menu_fw_update.hpp"
#include "screen_menu_lan_settings.hpp"
#include "screen_menu_hw_setup.hpp"
#include "screen_menu_eeprom.hpp"
#include "screen_menu_footer_settings.hpp"
#include "screen_prusa_link.hpp"
#include "screen_menu_connect.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "screen_menu_network.hpp"
#include "screen_menu_eeprom_diagnostics.hpp"
#include "screen_menu_fail_stat.hpp"
#include "screen_menu_user_interface.hpp"
#include "screen_menu_lang_and_time.hpp"
#include "screen_menu_hardware.hpp"
#include "screen_menu_hardware_tune.hpp"
#include "screen_menu_system.hpp"
#include "screen_menu_statistics.hpp"
#include "screen_menu_factory_reset.hpp"
#include "screen_menu_error_test.hpp"
#include "screen_menu_input_shaper.hpp"

#include <printers.h>

#if HAS_SELFTEST()
    #include "screen_menu_diagnostics.hpp"
    #include <option/has_selftest_snake.h>
    #if HAS_SELFTEST_SNAKE()
        #include "screen_menu_selftest_snake.hpp"
    #endif
#endif

#include "gui/test/screen_menu_test.hpp"

#include <option/has_side_leds.h>
#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif

#include <config_store/store_instance.hpp>

/*****************************************************************************/
// MI_VERSION_INFO
MI_VERSION_INFO::MI_VERSION_INFO()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_VERSION_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuVersionInfo>);
}

/*****************************************************************************/
// MI_SENSOR_INFO
MI_SENSOR_INFO::MI_SENSOR_INFO()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_SENSOR_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuSensorInfo>);
}

/*****************************************************************************/
MI_ODOMETER::MI_ODOMETER()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_ODOMETER::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuOdometer>);
}

/*****************************************************************************/
// MI_FILAMENT
MI_FILAMENT::MI_FILAMENT()
    : WI_LABEL_t(_(label), &img::spool_16x16, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_FILAMENT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilament>);
}

/*****************************************************************************/
// MI_SYS_INFO
MI_SYS_INFO::MI_SYS_INFO()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_SYS_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_sysinfo_data_t>);
}

// MI_STATISTIC_disabled
MI_STATISTIC_disabled::MI_STATISTIC_disabled()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_FAIL_STAT
MI_FAIL_STAT::MI_FAIL_STAT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FAIL_STAT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFailStat>);
}

/*****************************************************************************/
// MI_SUPPORT_disabled
MI_SUPPORT_disabled::MI_SUPPORT_disabled()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) {
}

/*****************************************************************************/
// MI_TEMPERATURE
MI_TEMPERATURE::MI_TEMPERATURE()
    : WI_LABEL_t(_(label), &img::temperature_16x16, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_TEMPERATURE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTemperature>);
}

/*****************************************************************************/
// MI_MOVE_AXIS
MI_MOVE_AXIS::MI_MOVE_AXIS()
    : WI_LABEL_t(_(label), &img::move_16x16, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_MOVE_AXIS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMove>);
}

/*****************************************************************************/
// MI_SERVICE
MI_SERVICE::MI_SERVICE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SERVICE::click(IWindowMenu & /*window_menu*/) {
    // screen_open(get_scr_menu_service()->id);
}

/*****************************************************************************/
// MI_ERROR_TEST
#if DEVELOPER_MODE()
MI_ERROR_TEST::MI_ERROR_TEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_ERROR_TEST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuErrorTest>);
}
#endif /*DEVELOPMENT_ITEMS()*/

/*****************************************************************************/
// MI_TEST
MI_TEST::MI_TEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_TEST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTest>);
}

/*****************************************************************************/
// MI_FW_UPDATE
MI_FW_UPDATE::MI_FW_UPDATE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_FW_UPDATE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFwUpdate>);
}

/*****************************************************************************/
// MI_METRICS_SETTINGS
MI_METRICS_SETTINGS::MI_METRICS_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    SetIconId(&img::speed_16x16);
}

void MI_METRICS_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuMetricsSettings>);
}

/*****************************************************************************/
// MI_ETH_SETTINGS
MI_ETH_SETTINGS::MI_ETH_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    SetIconId(&img::lan_16x16);
}

void MI_ETH_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuEthernetSettings>);
}

/*****************************************************************************/
// MI_WIFI_SETTINGS
MI_WIFI_SETTINGS::MI_WIFI_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    SetIconId(&img::wifi_16x16);
}

void MI_WIFI_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuWifiSettings>);
}

/*****************************************************************************/
// MI_MESSAGES
MI_MESSAGES::MI_MESSAGES()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_MESSAGES::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<screen_messages_data_t>);
}

/*****************************************************************************/
// MI_HW_SETUP
MI_HW_SETUP::MI_HW_SETUP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::yes, expands_t::yes) {
}

void MI_HW_SETUP::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHwSetup>);
}

/*****************************************************************************/
// MI_CURRENT_PROFILE
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
// MI_EEPROM
MI_EEPROM::MI_EEPROM()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_EEPROM::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuEeprom>);
}

/*****************************************************************************/
// MI_DEVHASH_IN_QR
MI_DEVHASH_IN_QR::MI_DEVHASH_IN_QR()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().devhash_in_qr.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_DEVHASH_IN_QR::OnChange(size_t old_index) {
    config_store().devhash_in_qr.set(!old_index);
}

/*****************************************************************************/
// MI_FOOTER_SETTINGS
MI_FOOTER_SETTINGS::MI_FOOTER_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_FOOTER_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFooterSettings>);
}

#ifdef HAS_TMC_WAVETABLE
MI_WAVETABLE_XYZ::MI_WAVETABLE_XYZ()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().tmc_wavetable_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}
void MI_WAVETABLE_XYZ::OnChange(size_t old_index) {
    /// enable
    old_index ? tmc_disable_wavetable(true, true, true) : tmc_enable_wavetable(true, true, true);
    config_store().tmc_wavetable_enabled.set(!old_index);
}
#endif

/*****************************************************************************/
// MI_FOOTER_SETTINGS_ADV
MI_FOOTER_SETTINGS_ADV::MI_FOOTER_SETTINGS_ADV()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_FOOTER_SETTINGS_ADV::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFooterSettingsAdv>);
}

/*****************************************************************************/
// MI_PRUSALINK
MI_PRUSALINK::MI_PRUSALINK()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_PRUSALINK::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuPrusaLink>);
}

/**********************************************************************************************/
// MI_DIAGNOSTICS

MI_DIAGNOSTICS::MI_DIAGNOSTICS()
    : WI_LABEL_t(_(label), nullptr,
#if HAS_SELFTEST()
        is_enabled_t::yes
#else
        is_enabled_t::no
#endif
        ,
        is_hidden_t::no, expands_t::yes) {
}

void MI_DIAGNOSTICS::click(IWindowMenu & /*window_menu*/) {
#if HAS_SELFTEST()
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuDiagnostics>);
#endif
}

/**********************************************************************************************/
// MI_USER_INTERFACE

MI_USER_INTERFACE::MI_USER_INTERFACE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_USER_INTERFACE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuUserInterface>);
}

/**********************************************************************************************/
// MI_LANG_AND_TIME

MI_LANG_AND_TIME::MI_LANG_AND_TIME()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_LANG_AND_TIME::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuLangAndTime>);
}

/*****************************************************************************/
// MI_PRUSALINK
MI_PRUSA_CONNECT::MI_PRUSA_CONNECT()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

/**********************************************************************************************/
// MI_PRUSA_CONNECT
void MI_PRUSA_CONNECT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuConnect>);
}

/**********************************************************************************************/
// MI_LOAD_SETTINGS

MI_LOAD_SETTINGS::MI_LOAD_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_LOAD_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    // FIXME: Some error handling/reporting
    // TODO: Loading other things than just network
    if (netdev_load_ini_to_eeprom()) {
        notify_reconfigure();
    }

// FIXME: Error handling
#if BUDDY_ENABLE_CONNECT()
    connect_client::MarlinPrinter::load_cfg_from_ini();
#endif
}

/**********************************************************************************************/
// MI_NETWORK

MI_NETWORK::MI_NETWORK()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_NETWORK::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuNetwork>);
}

/**********************************************************************************************/
// MI_HARDWARE

MI_HARDWARE::MI_HARDWARE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_HARDWARE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHardware>);
}

/**********************************************************************************************/
// MI_HARDWARE_TUNE

MI_HARDWARE_TUNE::MI_HARDWARE_TUNE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_HARDWARE_TUNE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHardwareTune>);
}

/**********************************************************************************************/
// MI_EXPERIMENTAL_SETTINGS
MI_EXPERIMENTAL_SETTINGS::MI_EXPERIMENTAL_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {
}

void MI_EXPERIMENTAL_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>);
}

/**********************************************************************************************/
// MI_SYSTEM

MI_SYSTEM::MI_SYSTEM()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_SYSTEM::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuSystem>);
}

/**********************************************************************************************/
// MI_PRINT_STATISTICS

MI_PRINT_STATISTICS::MI_PRINT_STATISTICS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_PRINT_STATISTICS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuStatistics>);
}

/**********************************************************************************************/
// MI_INFO

MI_INFO::MI_INFO()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_INFO::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuInfo>);
}

/**********************************************************************************************/
// MI_EEPROM_DIAGNOSTICS
MI_EEPROM_DIAGNOSTICS::MI_EEPROM_DIAGNOSTICS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_EEPROM_DIAGNOSTICS::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuEepromDiagnostics>);
}

/**********************************************************************************************/
// MI_USB_MSC_ENABLE
MI_USB_MSC_ENABLE::MI_USB_MSC_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().usb_msc_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_USB_MSC_ENABLE::OnChange(size_t old_index) {
    config_store().usb_msc_enabled.set(!old_index);
}
#if HAS_LEDS()
/**********************************************************************************************/
// MI_LEDS_ENABLE
MI_LEDS_ENABLE::MI_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(Animator_LCD_leds().animator_state(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_LEDS_ENABLE::OnChange(size_t old_index) {
    if (old_index) {
        Animator_LCD_leds().pause_animator();
    } else {
        Animator_LCD_leds().start_animator();
    }
}
#endif

#if HAS_SIDE_LEDS()
/**********************************************************************************************/
// MI_SIDE_LEDS_ENABLE
MI_SIDE_LEDS_ENABLE::MI_SIDE_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().side_leds_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_SIDE_LEDS_ENABLE::OnChange(size_t old_index) {
    leds::side_strip_control.SetEnable(!old_index);
    config_store().side_leds_enabled.set(!old_index);
}
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
/**********************************************************************************************/
// MI_TOOL_LEDS_ENABLE
MI_TOOL_LEDS_ENABLE::MI_TOOL_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().tool_leds_enabled.get(), _(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}
void MI_TOOL_LEDS_ENABLE::OnChange(size_t old_index) {
    HOTEND_LOOP() {
        prusa_toolchanger.getTool(e).set_cheese_led(!old_index ? 0xff : 0x00, 0x00);
    }
    config_store().tool_leds_enabled.set(!old_index);
}

/*****************************************************************************/
// MI_TOOLS_SETUP
MI_TOOLS_SETUP::MI_TOOLS_SETUP()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_TOOLS_SETUP::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuTools>);
}
#endif

// MI_TRIGGER_POWER_PANIC
MI_TRIGGER_POWER_PANIC::MI_TRIGGER_POWER_PANIC()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::no) {
}

void MI_TRIGGER_POWER_PANIC::click([[maybe_unused]] IWindowMenu &windowMenu) {
    // this is normally supposed to be called from ISR, but since disables IRQ so it works fine even outside of ISR
    power_panic::ac_fault_isr();
}

#if ENABLED(PRUSA_TOOLCHANGER)
/*****************************************************************************/
MI_PICK_PARK_TOOL::MI_PICK_PARK_TOOL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_PICK_PARK_TOOL::click(IWindowMenu & /*window_menu*/) {
    ToolActionBox<ToolBox::MenuPickPark>();
}

/*****************************************************************************/
MI_CALIBRATE_DOCK::MI_CALIBRATE_DOCK()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_CALIBRATE_DOCK::click(IWindowMenu & /*window_menu*/) {
    ToolActionBox<ToolBox::MenuCalibrateDock>();
    Screens::Access()->Get()->Validate();
}
#endif

#if HAS_SELFTEST_SNAKE()
/**********************************************************************************************/
// MI_SELFTEST_SNAKE

MI_SELFTEST_SNAKE::MI_SELFTEST_SNAKE()
    : WI_LABEL_t(_(label), &img::calibrate_white_16x16,
    #if HAS_SELFTEST()
        is_enabled_t::yes
    #else
        is_enabled_t::no
    #endif
        ,
        is_hidden_t::no, expands_t::yes) {
}

void MI_SELFTEST_SNAKE::click(IWindowMenu & /*window_menu*/) {
    #if HAS_SELFTEST() && HAS_SELFTEST_SNAKE()
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuSTSCalibrations>);
    #else
    bsod("Invalid configuration for this menu item");
    #endif
}
#endif

/**********************************************************************************************/
MI_OPEN_FACTORY_RESET::MI_OPEN_FACTORY_RESET()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_OPEN_FACTORY_RESET::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFactoryReset>);
}

MI_INPUT_SHAPER::MI_INPUT_SHAPER()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_INPUT_SHAPER::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuInputShaper>);
}
