#include "MItem_menus.hpp"
#include "ScreenHandler.hpp"
#include <option/buddy_enable_connect.h>
#include "screen_messages.hpp"
#include "translator.hpp"
#include "screen_menu_fan_info.hpp"
#include "screen_menu_board_info.hpp"
#include "screen_menu_temperature_and_fans.hpp"
#include "screen_menu_move.hpp"
#include "screen_menu_sensor_info.hpp"
#include "screen_menu_version_info.hpp"
#include "screen_menu_metrics.hpp"
#include "screen_menu_network_status.hpp"
#include "screen_menu_network_settings.hpp"
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
#include "screen_menu_gcode_checks.hpp"
#include "screen_help_fw_update.hpp"
#include <option/has_wastebin_fill_tracking.h>
#if HAS_WASTEBIN_FILL_TRACKING()
    #include "screen_menu_wastebin.hpp"
#endif
#include "screen_menu_system.hpp"
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
#include <gui/screen/screen_factory_reset.hpp>

#include <option/has_esp.h>
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#include <option/has_indx.h>

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

#if HAS_CHAMBER_FILTRATION_API()
    #include <gui/screen/screen_chamber_filtration.hpp>
#endif

#if HAS_MMU2()
    #include <gui/screen/screen_hw_mmu.hpp>
#endif

#if HAS_E2EE_SUPPORT()
    #include "screen_menu_e2ee.hpp"
    #include "ScreenHandler.hpp"
#endif

#if HAS_LIGHTS_MENU()
    #include <screen/screen_menu_lights.hpp>
#endif

#if HAS_PHASE_STEPPING()
    #include "screen_menu_phase_stepping.hpp"
#endif

#include <option/has_touch.h>
#include <option/development_items.h>
#if HAS_TOUCH() && DEVELOPMENT_ITEMS()
    #include "screen_touch_playground.hpp"
#endif

#if DEVELOPMENT_ITEMS()
    #include "screen_menu_advanced_footer_settings.hpp"
    #include "screen_menu_development.hpp"
    #include "screen_menu_error_test.hpp"
    #include <gui/screen/screen_printer_type_changed.hpp>
#endif

#include <config_store/store_instance.hpp>

MI_SCREEN_BASE::MI_SCREEN_BASE(ScreenFactory::Creator screen_ctor, const char *label)
    : MI_SCREEN_BASE(screen_ctor, label, nullptr) {}

MI_SCREEN_BASE::MI_SCREEN_BASE(ScreenFactory::Creator::Func screen_ctor, const char *label)
    : MI_SCREEN_BASE(screen_ctor, label, nullptr) {}

MI_SCREEN_BASE::MI_SCREEN_BASE(ScreenFactory::Creator screen_ctor, const char *label, const img::Resource *icon, is_hidden_t is_hidden)
    : IWindowMenuItem(_(label), icon, is_enabled_t::yes, is_hidden, expands_t::yes)
    , screen_ctor_(screen_ctor) {}

#if DEVELOPMENT_ITEMS()
MI_SCREEN_BASE::MI_SCREEN_BASE(ScreenFactory::Creator screen_ctor, const string_view_utf8 &label, is_hidden_t is_hidden)
    : IWindowMenuItem(label, nullptr, is_enabled_t::yes, is_hidden, expands_t::yes)
    , screen_ctor_(screen_ctor) {}
#endif

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
template struct MI_SCREEN_CTOR<ScreenMenuFanInfo>;
template struct MI_SCREEN_CTOR<ScreenMenuVersionInfo>;
template struct MI_SCREEN_CTOR<ScreenMenuSensorInfo>;
template struct MI_SCREEN_CTOR<ScreenMenuFailStat>;
template struct MI_SCREEN_CTOR<ScreenMenuTemperatureAndFans>;
template struct MI_SCREEN_CTOR<ScreenMenuMove>;
template struct MI_SCREEN_CTOR<ScreenMenuMetricsSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuEthernetSettings>;
#if HAS_ESP()
template struct MI_SCREEN_CTOR<ScreenMenuWifiSettings>;
#endif
template struct MI_SCREEN_CTOR<screen_messages_data_t>;
template struct MI_SCREEN_CTOR<ScreenMenuConnect>;
template struct MI_SCREEN_CTOR<ScreenMenuPrusaLink>;
template struct MI_SCREEN_CTOR<ScreenMenuFooterSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuExperimentalSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuUserInterface>;
template struct MI_SCREEN_CTOR<ScreenMenuLangAndTime>;
template struct MI_SCREEN_CTOR<ScreenMenuNetwork>;
template struct MI_SCREEN_CTOR<ScreenMenuNetworkStatus>;
template struct MI_SCREEN_CTOR<ScreenMenuHardware>;
template struct MI_SCREEN_CTOR<ScreenMenuHardwareTune>;
template struct MI_SCREEN_CTOR<ScreenMenuGcodeChecks>;
template struct MI_SCREEN_CTOR<ScreenHelpFWUpdate>;
#if HAS_WASTEBIN_FILL_TRACKING()
template struct MI_SCREEN_CTOR<ScreenMenuWastebin>;
#endif
template struct MI_SCREEN_CTOR<ScreenMenuSystem>;
template struct MI_SCREEN_CTOR<ScreenMenuInfo>;
template struct MI_SCREEN_CTOR<ScreenFactoryReset>;
template struct MI_SCREEN_CTOR<ScreenMenuInputShaper>;
template struct MI_SCREEN_CTOR<ScreenPrinterSetup>;

#if DEVELOPMENT_ITEMS()
template struct MI_SCREEN_CTOR<ScreenMenuAdvancedFooterSettings>;
template struct MI_SCREEN_CTOR<ScreenMenuDevelopment>;
template struct MI_SCREEN_CTOR<ScreenMenuErrorTest>;
template struct MI_SCREEN_CTOR<ScreenPrinterTypeChanged>;
#endif

#if HAS_TRANSLATIONS()
template struct MI_SCREEN_CTOR<ScreenMenuLanguages>;
#endif

#if HAS_PHASE_STEPPING()
template struct MI_SCREEN_CTOR<ScreenMenuPhaseStepping>;
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

template struct MI_SCREEN_CTOR<ScreenMenuBoardInfo>;

#if HAS_LIGHTS_MENU()
template struct MI_SCREEN_CTOR<ScreenMenuLights>;
#endif

#if HAS_TOUCH() && DEVELOPMENT_ITEMS()
template struct MI_SCREEN_CTOR<ScreenTouchPlayground>;
#endif

/**********************************************************************************************/
// MI_SERIAL_PRINTING_SCREEN_ENABLE
MI_SERIAL_PRINTING_SCREEN_ENABLE::MI_SERIAL_PRINTING_SCREEN_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().serial_print_screen_enabled.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_SERIAL_PRINTING_SCREEN_ENABLE::OnChange(size_t old_index) {
    config_store().serial_print_screen_enabled.set(!old_index);
}

// TODO: this would be better to split into separate menu items between singletool printers and the toolchanger ones.
// * MI_TOOLHEAD_SETTINGS
MI_TOOLHEAD_SETTINGS::MI_TOOLHEAD_SETTINGS()
    : IWindowMenuItem(
#if HAS_TOOLCHANGER()
        prusa_toolchanger.is_toolchanger_enabled() ? _("Tools") : _("Toolhead"),
#else
        _("Printhead"),
#endif
        nullptr,
#if HAS_TOOLCHANGER()
        prusa_toolchanger.get_num_enabled_tools() > 0 ? is_enabled_t::yes : is_enabled_t::no,
#else
        is_enabled_t::yes,
#endif
        is_hidden_t::no, expands_t::yes) {
}

void MI_TOOLHEAD_SETTINGS::click(IWindowMenu &) {
    ScreenFactory::Creator screen = ScreenFactory::Screen<ScreenToolheadDetail>;

#if HAS_TOOLCHANGER()
    screen = ScreenFactory::Screen<ScreenToolheadSettingsList>;
#endif

#if PRINTER_IS_PRUSA_XL()
    const bool is_singletool_xl = !prusa_toolchanger.is_toolchanger_enabled() && PhysicalToolIndex::single_enabled_tool().has_value();
    if (is_singletool_xl) {
        screen = ScreenFactory::Screen<ScreenToolheadDetail>;
    }
#endif

    Screens::Access()->Open(screen);
}

#if HAS_CHAMBER_FILTRATION_API()
template struct MI_SCREEN_CTOR<ScreenChamberFiltration>;
#endif

#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>

MI_HW_MMU::MI_HW_MMU()
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH("MMU"), nullptr, is_enabled_t::yes, MMU2::mmu2.Enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_HW_MMU::click(IWindowMenu &) {
    Screens::Access()->Open<ScreenMenuHwMmu>();
}
#endif

#if HAS_E2EE_SUPPORT()
template struct MI_SCREEN_CTOR<ScreenMenuE2ee>;
#endif
