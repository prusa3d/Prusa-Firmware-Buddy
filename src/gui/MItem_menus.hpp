#pragma once

#include "WindowMenuItems.hpp"
#include "i18n.h"
#include <option/has_toolchanger.h>
#include <option/has_side_leds.h>
#include <option/has_filament_sensors_menu.h>
#include <option/has_leds.h>
#include <option/has_sheet_profiles.h>
#include <option/developer_mode.h>
#include <option/has_translations.h>
#include <img_resources.hpp>
#include <ScreenFactory.hpp>

class MI_SCREEN_BASE : public IWindowMenuItem {
protected:
    // Two constructors for flash saving (so that we don't need to pass that many parameters)
    MI_SCREEN_BASE(ScreenFactory::Creator::Func screen_ctor, const char *label);
    MI_SCREEN_BASE(ScreenFactory::Creator::Func screen_ctor, const char *label, const img::Resource *icon, is_hidden_t is_hidden = is_hidden_t::no);

    void click(IWindowMenu &) final;

private:
    const ScreenFactory::Creator::Func screen_ctor_;
};

template <typename T>
struct MI_SCREEN_CTOR {
    static ScreenFactory::Creator::Func get();
};

/// Usage:
/// - Add here: using MI_XXX = MI_SCREEN<N_("Lavbel"), class ScreenClass>;
/// - Include the relevant screen header in the cpp
/// - Instantiate template struct MI_SCREEN_CTOR<ScreenClass>; in the cpp
template <auto label_, class Screen_, auto... args>
class MI_SCREEN final : public MI_SCREEN_BASE {
public:
    // Implemented in the cpp file
    inline MI_SCREEN()
        : MI_SCREEN_BASE(MI_SCREEN_CTOR<Screen_>::get(), label_, args...) {}
};

using MI_FILAMENT_MANAGEMENT = MI_SCREEN<N_("Manage Filaments"), class ScreenFilamentManagement>;
using MI_EDIT_FILAMENTS = MI_SCREEN<N_("Edit Filaments"), class ScreenFilamentManagementList>;
using MI_REORDER_FILAMENTS = MI_SCREEN<N_("Reorder Filaments"), class ScreenFilamentsReorder>;
using MI_FILAMENTS_VISIBILITY = MI_SCREEN<N_("Enable Filaments"), class ScreenFilamentsVisibility>;
using MI_VERSION_INFO = MI_SCREEN<N_("Version Info"), class ScreenMenuVersionInfo>;
using MI_SENSOR_INFO = MI_SCREEN<N_("Sensor Info"), class ScreenMenuSensorInfo>;
using MI_ODOMETER = MI_SCREEN<N_("Statistics"), class ScreenMenuOdometer>;
using MI_SYS_INFO = MI_SCREEN<N_("System Info"), class screen_sysinfo_data_t>;
using MI_FAIL_STAT = MI_SCREEN<N_("Fail Stats"), class ScreenMenuFailStat>;
using MI_TEMPERATURE = MI_SCREEN<N_("Temperature"), class ScreenMenuTemperature, &img::temperature_16x16>;
using MI_MOVE_AXIS = MI_SCREEN<N_("Move Axis"), class ScreenMenuMove, &img::move_16x16>;
using MI_FW_UPDATE = MI_SCREEN<N_("FW Update"), class ScreenMenuFwUpdate, nullptr, is_hidden_t::dev>;
using MI_METRICS_SETTINGS = MI_SCREEN<N_("Metrics & Log"), class ScreenMenuMetricsSettings>;
using MI_ETH_SETTINGS = MI_SCREEN<N_("Ethernet"), class ScreenMenuEthernetSettings, &img::lan_16x16>;
using MI_WIFI_SETTINGS = MI_SCREEN<N_("Wi-Fi"), class ScreenMenuWifiSettings, &img::wifi_16x16>;
using MI_MESSAGES = MI_SCREEN<N_("Message History"), class screen_messages_data_t>;
using MI_PRUSA_CONNECT = MI_SCREEN<N_("Prusa Connect"), class ScreenMenuConnect>;
using MI_PRUSALINK = MI_SCREEN<N_("PrusaLink"), class ScreenMenuPrusaLink>;
using MI_EEPROM = MI_SCREEN<N_("EEPROM"), class ScreenMenuEeprom, nullptr, is_hidden_t::dev>;
using MI_FOOTER_SETTINGS = MI_SCREEN<N_("Footer"), class ScreenMenuFooterSettings>;
using MI_FOOTER_SETTINGS_ADV = MI_SCREEN<N_("Advanced"), class ScreenMenuFooterSettingsAdv, nullptr, is_hidden_t::dev>;
using MI_EXPERIMENTAL_SETTINGS = MI_SCREEN<N_("Experimental Settings"), class ScreenMenuExperimentalSettings, nullptr, is_hidden_t::dev>;
using MI_USER_INTERFACE = MI_SCREEN<N_("User Interface"), class ScreenMenuUserInterface>;
using MI_LANG_AND_TIME = MI_SCREEN<N_("Language & Time"), class ScreenMenuLangAndTime>;
using MI_NETWORK = MI_SCREEN<N_("Network"), class ScreenMenuNetwork>;
using MI_NETWORK_STATUS = MI_SCREEN<N_("Network Status"), class ScreenMenuNetworkStatus>;
using MI_HARDWARE = MI_SCREEN<N_("Hardware"), class ScreenMenuHardware>;
using MI_HARDWARE_TUNE = MI_SCREEN<N_("Hardware"), class ScreenMenuHardwareTune, nullptr, is_hidden_t::dev>;
using MI_SYSTEM = MI_SCREEN<N_("System"), class ScreenMenuSystem>;
using MI_PRINT_STATISTICS = MI_SCREEN<N_("Print Statistics"), class ScreenMenuStatistics>;
using MI_INFO = MI_SCREEN<N_("Info"), class ScreenMenuInfo>;
using MI_OPEN_FACTORY_RESET = MI_SCREEN<N_("Factory Reset"), class ScreenMenuFactoryReset>;
using MI_INPUT_SHAPER = MI_SCREEN<N_("Input Shaper"), class ScreenMenuInputShaper>;

#if DEVELOPER_MODE()
using MI_ERROR_TEST = MI_SCREEN<N_("Test Errors"), class ScreenMenuErrorTest, nullptr, is_hidden_t::dev>;
#endif

#if HAS_TRANSLATIONS()
using MI_LANGUAGE = MI_SCREEN<N_("Language"), class ScreenMenuLanguages, &img::language_16x16>;
#endif

#if HAS_SHEET_PROFILES()
using MI_STEEL_SHEETS = MI_SCREEN<N_("Steel Sheets"), class ScreenMenuSteelSheets>;
#endif

#if HAS_FILAMENT_SENSORS_MENU()
using MI_FILAMENT_SENSORS = MI_SCREEN<N_("Filament sensors"), class ScreenMenuFilamentSensors>;
#endif

#if HAS_SELFTEST()
using MI_SELFTEST_SNAKE = MI_SCREEN<N_("Calibrations & Tests"), class ScreenMenuSTSCalibrations, &img::calibrate_white_16x16>;
#endif

#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
using MI_BED_LEVEL_CORRECTION = MI_SCREEN<N_("Bed Level Correction"), class ScreenMenuBedLevelCorrection>;
#endif
