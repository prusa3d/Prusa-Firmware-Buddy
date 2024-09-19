#include "MItem_menus.hpp"
#include "ScreenHandler.hpp"
#include "str_utils.hpp"
#include <option/buddy_enable_connect.h>
#include "screen_sysinf.hpp"
#include "screen_qr_error.hpp"
#include "screen_messages.hpp"
#include "translator.hpp"
#include "screen_menu_temperature.hpp"
#include "screen_menu_move.hpp"
#include "screen_menu_sensor_info.hpp"
#include "screen_menu_odometer.hpp"
#include "screen_menu_version_info.hpp"
#include "screen_menu_metrics.hpp"
#include "screen_menu_fw_update.hpp"
#include "screen_menu_network_status.hpp"
#include "screen_menu_network_settings.hpp"
#include "screen_menu_eeprom.hpp"
#include "screen_menu_footer_settings.hpp"
#include "screen_prusa_link.hpp"
#include "screen_menu_connect.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "screen_menu_network.hpp"
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
#include <screen_menu_languages.hpp>
#include <screen_menu_info.hpp>
#include <screen_menu_control.hpp>
#include <screen_menu_settings.hpp>
#include <screen_menu_tune.hpp>
#include <screen_menu_filament.hpp>
#include <screen_printer_setup.hpp>

#include <screen/filament/screen_filament_management.hpp>
#include <screen/filament/screen_filament_management_list.hpp>
#include <screen/filament/screen_filaments_reorder.hpp>
#include <screen/filament/screen_filaments_visibility.hpp>
#include <screen/toolhead/screen_toolhead_settings.hpp>

#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
    #include <screen_menu_bed_level_correction.hpp>
#endif

#if HAS_SELFTEST()
    #include "screen_menu_selftest_snake.hpp"
#endif

#if HAS_FILAMENT_SENSORS_MENU()
    #include "screen_menu_filament_sensors.hpp"
#endif

#if HAS_SHEET_PROFILES()
    #include <screen_menu_steel_sheets.hpp>
#endif

#include <config_store/store_instance.hpp>

MI_SCREEN_BASE::MI_SCREEN_BASE(ScreenFactory::Creator::Func screen_ctor, const char *label)
    : MI_SCREEN_BASE(screen_ctor, label, nullptr) {}

MI_SCREEN_BASE::MI_SCREEN_BASE(ScreenFactory::Creator::Func screen_ctor, const char *label, const img::Resource *icon, is_hidden_t is_hidden)
    : IWindowMenuItem(_(label), icon, is_enabled_t::yes, is_hidden, expands_t::yes)
    , screen_ctor_(screen_ctor) {}

void MI_SCREEN_BASE::click(IWindowMenu &) {
    Screens::Access()->Open(screen_ctor_);
}

template <typename T>
ScreenFactory::Creator::Func MI_SCREEN_CTOR<T>::get() {
    return ScreenFactory::Screen<T>;
}

template struct MI_SCREEN_CTOR<ScreenFilamentManagement>;
template struct MI_SCREEN_CTOR<ScreenFilamentManagementList>;
template struct MI_SCREEN_CTOR<ScreenFilamentsReorder>;
template struct MI_SCREEN_CTOR<ScreenFilamentsVisibility>;
template struct MI_SCREEN_CTOR<ScreenMenuVersionInfo>;
template struct MI_SCREEN_CTOR<ScreenMenuSensorInfo>;
template struct MI_SCREEN_CTOR<ScreenMenuOdometer>;
template struct MI_SCREEN_CTOR<screen_sysinfo_data_t>;
template struct MI_SCREEN_CTOR<ScreenMenuFailStat>;
template struct MI_SCREEN_CTOR<ScreenMenuTemperature>;
template struct MI_SCREEN_CTOR<ScreenMenuMove>;
template struct MI_SCREEN_CTOR<ScreenMenuFwUpdate>;
template struct MI_SCREEN_CTOR<ScreenMenuMetricsSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuEthernetSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuWifiSettings>;
template struct MI_SCREEN_CTOR<screen_messages_data_t>;
template struct MI_SCREEN_CTOR<ScreenMenuConnect>;
template struct MI_SCREEN_CTOR<ScreenMenuPrusaLink>;
template struct MI_SCREEN_CTOR<ScreenMenuEeprom>;
template struct MI_SCREEN_CTOR<ScreenMenuFooterSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuFooterSettingsAdv>;
template struct MI_SCREEN_CTOR<ScreenMenuExperimentalSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuUserInterface>;
template struct MI_SCREEN_CTOR<ScreenMenuLangAndTime>;
template struct MI_SCREEN_CTOR<ScreenMenuNetwork>;
template struct MI_SCREEN_CTOR<ScreenMenuNetworkStatus>;
template struct MI_SCREEN_CTOR<ScreenMenuHardware>;
template struct MI_SCREEN_CTOR<ScreenMenuHardwareTune>;
template struct MI_SCREEN_CTOR<ScreenMenuSystem>;
template struct MI_SCREEN_CTOR<ScreenMenuStatistics>;
template struct MI_SCREEN_CTOR<ScreenMenuInfo>;
template struct MI_SCREEN_CTOR<ScreenMenuFactoryReset>;
template struct MI_SCREEN_CTOR<ScreenMenuInputShaper>;
template struct MI_SCREEN_CTOR<ScreenPrinterSetup>;

#if DEVELOPER_MODE()
template struct MI_SCREEN_CTOR<ScreenMenuErrorTest>;
#endif

#if HAS_TRANSLATIONS()
template struct MI_SCREEN_CTOR<ScreenMenuLanguages>;
#endif

#if HAS_SHEET_PROFILES()
template struct MI_SCREEN_CTOR<ScreenMenuSteelSheets>;
#endif

#if HAS_FILAMENT_SENSORS_MENU()
template struct MI_SCREEN_CTOR<ScreenMenuFilamentSensors>;
#endif

#if HAS_SELFTEST()
template struct MI_SCREEN_CTOR<ScreenMenuSTSCalibrations>;
#endif

#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
template struct MI_SCREEN_CTOR<ScreenMenuBedLevelCorrection>;
#endif

/**********************************************************************************************/
// MI_SERIAL_PRINTING_SCREEN_ENABLE
MI_SERIAL_PRINTING_SCREEN_ENABLE::MI_SERIAL_PRINTING_SCREEN_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().serial_print_screen_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_SERIAL_PRINTING_SCREEN_ENABLE::OnChange(size_t old_index) {
    config_store().serial_print_screen_enabled.set(!old_index);
}

// * MI_TOOLHEAD_SETTINGS
MI_TOOLHEAD_SETTINGS::MI_TOOLHEAD_SETTINGS()
    : IWindowMenuItem(
#if HAS_TOOLCHANGER()
        prusa_toolchanger.is_toolchanger_enabled() ? _("Tools") : _("Toolhead"),
#else
        _("Printhead"),
#endif
        nullptr,
        is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_TOOLHEAD_SETTINGS::click(IWindowMenu &) {
    ScreenFactory::Creator screen = ScreenFactory::Screen<ScreenToolheadDetail>;

#if HAS_TOOLCHANGER()
    if (prusa_toolchanger.is_toolchanger_enabled() && prusa_toolchanger.get_num_enabled_tools() > 1) {
        screen = ScreenFactory::Screen<ScreenToolheadSettingsList>;
    }
#endif

    Screens::Access()->Open(screen);
}
