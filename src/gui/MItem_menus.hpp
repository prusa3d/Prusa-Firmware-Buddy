#pragma once

#include "WindowMenuItems.hpp"
#include "i18n.h"
#include <str_utils.hpp>
#include <option/has_side_leds.h>
#include <option/has_filament_sensors_menu.h>
#include <option/has_leds.h>
#include <option/has_phase_stepping.h>
#include <option/has_sheet_profiles.h>
#include <option/development_items.h>
#include <option/has_translations.h>
#include <option/has_chamber_filtration_api.h>
#include <option/has_mmu2.h>
#include <option/has_e2ee_support.h>
#include <option/has_wastebin_fill_tracking.h>
#include <option/has_lights_menu.h>
#include <img_resources.hpp>
#include <ScreenFactory.hpp>

#include <option/has_esp.h>

class MI_SCREEN_BASE : public IWindowMenuItem {
protected:
    MI_SCREEN_BASE(ScreenFactory::Creator screen_ctor, const char *label, const img::Resource *icon, is_hidden_t is_hidden = is_hidden_t::no);

#if DEVELOPMENT_ITEMS()
    // used by MI_SCREEN_DEV
    MI_SCREEN_BASE(ScreenFactory::Creator screen_ctor, const string_view_utf8 &label, is_hidden_t is_hidden);
#endif

    // This saves flash (so that we don't need to pass that many parameters)
    MI_SCREEN_BASE(ScreenFactory::Creator::Func screen_ctor, const char *label);
    MI_SCREEN_BASE(ScreenFactory::Creator screen_ctor, const char *label);

    void click(IWindowMenu &) final;

private:
    const ScreenFactory::Creator screen_ctor_;
};

template <typename T>
struct MI_SCREEN_CTOR {
    // Implemented in the cpp file
    static ScreenFactory::Creator::Func get();
};
/// Usage:
/// - Add here: using MI_XXX = MI_SCREEN<N_("Lavbel"), class ScreenClass>;
/// - Include the relevant screen header in the cpp
/// - Instantiate template struct MI_SCREEN_CTOR<ScreenClass>; in the cpp
template <auto label_, class Screen_, auto... args>
class MI_SCREEN final : public MI_SCREEN_BASE {
public:
    inline MI_SCREEN()
        : MI_SCREEN_BASE(MI_SCREEN_CTOR<Screen_>::get(), label_, args...) {}
};

using MI_FILAMENT_MANAGEMENT
    = MI_SCREEN<N_("Manage Filaments"), class ScreenFilamentManagement>;

using MI_EDIT_FILAMENTS
    = MI_SCREEN<N_("Edit Filaments"), class ScreenFilamentManagementList>;

using MI_REORDER_FILAMENTS
    = MI_SCREEN<N_("Reorder Filaments"), class ScreenFilamentsReorder>;

using MI_FILAMENTS_VISIBILITY
    = MI_SCREEN<N_("Enable Filaments"), class ScreenFilamentsVisibility>;

using MI_FAN_INFO
    = MI_SCREEN<N_("Fan Info"), class ScreenMenuFanInfo>;

using MI_BOARD_INFO
    = MI_SCREEN<N_("Board Info"), class ScreenMenuBoardInfo>;

using MI_VERSION_INFO
    = MI_SCREEN<N_("Version Info"), class ScreenMenuVersionInfo>;

using MI_SENSOR_INFO
    = MI_SCREEN<N_("Sensor Info"), class ScreenMenuSensorInfo>;

using MI_FAIL_STAT
    = MI_SCREEN<N_("Fail Stats"), class ScreenMenuFailStat>;

using MI_TEMPERATURE_AND_FANS
    = MI_SCREEN<N_("Temperature & Fans"), class ScreenMenuTemperatureAndFans, &img::temperature_16x16>;

using MI_MOVE_AXIS
    = MI_SCREEN<N_("Move Axis"), class ScreenMenuMove, &img::move_16x16>;

using MI_METRICS_SETTINGS
    = MI_SCREEN<N_("Metrics & Log"), class ScreenMenuMetricsSettings>;

using MI_ETH_SETTINGS
    = MI_SCREEN<N_("Ethernet"), class ScreenMenuEthernetSettings, &img::lan_16x16>;

#if HAS_ESP()
using MI_WIFI_SETTINGS
    = MI_SCREEN<N_("Wi-Fi"), class ScreenMenuWifiSettings, &img::wifi_16x16>;
#endif

using MI_MESSAGES
    = MI_SCREEN<N_("Message History"), class screen_messages_data_t>;

using MI_PRUSA_CONNECT
    = MI_SCREEN<N_("Prusa Connect"), class ScreenMenuConnect>;

using MI_PRUSALINK
    = MI_SCREEN<N_("PrusaLink"), class ScreenMenuPrusaLink>;

using MI_FOOTER_SETTINGS
    = MI_SCREEN<N_("Footer"), class ScreenMenuFooterSettings>;

using MI_USER_INTERFACE
    = MI_SCREEN<N_("User Interface"), class ScreenMenuUserInterface>;

using MI_LANG_AND_TIME
    = MI_SCREEN<N_("Language & Time"), class ScreenMenuLangAndTime>;

using MI_NETWORK
    = MI_SCREEN<N_("Network"), class ScreenMenuNetwork>;

using MI_NETWORK_STATUS
    = MI_SCREEN<N_("Network Status"), class ScreenMenuNetworkStatus>;

using MI_HARDWARE
    = MI_SCREEN<N_("Hardware"), class ScreenMenuHardware>;

using MI_GCODE_CHECKS
    = MI_SCREEN<N_("G-Code Checks"), class ScreenMenuGcodeChecks>;

using MI_HELP_FW_UPDATE
    = MI_SCREEN<N_("Firmware Update"), class ScreenHelpFWUpdate, &img::question_16x16>;

#if HAS_WASTEBIN_FILL_TRACKING()
using MI_WASTEBIN
    = MI_SCREEN<N_("Nozzle Cleaner"), class ScreenMenuWastebin>;
#endif

using MI_SYSTEM
    = MI_SCREEN<N_("System"), class ScreenMenuSystem>;

using MI_INFO
    = MI_SCREEN<N_("Info"), class ScreenMenuInfo>;

using MI_OPEN_FACTORY_RESET
    = MI_SCREEN<N_("Factory Reset"), class ScreenFactoryReset>;

using MI_INPUT_SHAPER
    = MI_SCREEN<N_("Input Shaper"), class ScreenMenuInputShaper>;

#if DEVELOPMENT_ITEMS()

/// Like MI_SCREEN, but for development-only screens
template <TemplateString label_, class Screen_>
class MI_SCREEN_DEV final : public MI_SCREEN_BASE {
public:
    MI_SCREEN_DEV()
        : MI_SCREEN_BASE { MI_SCREEN_CTOR<Screen_>::get(), string_view_utf8::MakeCPUFLASH(label_), is_hidden_t::dev } {}
};

using MI_ADVANCED_FOOTER
    = MI_SCREEN_DEV<"Advanced Footer"_tstr, class ScreenMenuAdvancedFooterSettings>;

using MI_HARDWARE_TUNE
    = MI_SCREEN_DEV<"Hardware"_tstr, class ScreenMenuHardwareTune>;

namespace screen_printer_setup_private {
class ScreenPrinterSetup;
}
using MI_PRINTER_SETUP
    = MI_SCREEN_DEV<"Printer Setup"_tstr, screen_printer_setup_private::ScreenPrinterSetup>;

using MI_DEVELOPMENT
    = MI_SCREEN_DEV<"Development"_tstr, class ScreenMenuDevelopment>;

using MI_EXPERIMENTAL_SETTINGS
    = MI_SCREEN_DEV<"Experimental Settings"_tstr, class ScreenMenuExperimentalSettings>;

using MI_ERROR_TEST
    = MI_SCREEN_DEV<"Test Errors"_tstr, class ScreenMenuErrorTest>;

using MI_TOUCH_PLAYGROUND
    = MI_SCREEN_DEV<"Touch Playground"_tstr, class ScreenTouchPlayground>;

using MI_PRINTER_TYPE_CHANGED
    = MI_SCREEN_DEV<"Printer Type Changed"_tstr, class ScreenPrinterTypeChanged>;

#endif

#if HAS_TRANSLATIONS()

using MI_LANGUAGE
    = MI_SCREEN<N_("Language"), class ScreenMenuLanguages, &img::language_16x16>;
#endif

#if HAS_PHASE_STEPPING()
using MI_PHASE_STEPPING_SCREEN
    = MI_SCREEN<
        N_("Phase Stepping"),
        class ScreenMenuPhaseStepping>;
#endif

#if HAS_SHEET_PROFILES()

using MI_STEEL_SHEETS
    = MI_SCREEN<N_("Steel Sheets"), class ScreenMenuSteelSheets>;
#endif

#if HAS_FILAMENT_SENSORS_MENU()

using MI_FILAMENT_SENSORS
    = MI_SCREEN<N_("Filament Sensors"), class ScreenMenuFilamentSensors>;
#endif

#if HAS_SELFTEST()

using MI_SELFTEST_SNAKE
    = MI_SCREEN<N_("Calibrations & Tests"), class ScreenMenuSTSCalibrations, &img::calibrate_white_16x16>;
#endif

#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()

using MI_BED_LEVEL_CORRECTION
    = MI_SCREEN<N_("Bed Level Correction"), class ScreenMenuBedLevelCorrection>;
#endif

#if HAS_LIGHTS_MENU()
using MI_LIGHTS
    = MI_SCREEN<N_("Lights"), class ScreenMenuLights>;
#endif

class MI_SERIAL_PRINTING_SCREEN_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Serial Printing Screen");

public:
    MI_SERIAL_PRINTING_SCREEN_ENABLE();
    virtual void OnChange(size_t old_index) override;
};

/// Opens toolhead 0 settings on single toolhead machines or submenu to select a toolhead on multi-toolhead machines
class MI_TOOLHEAD_SETTINGS final : public IWindowMenuItem {
public:
    MI_TOOLHEAD_SETTINGS();
    void click(IWindowMenu &) override;
};

#if HAS_CHAMBER_FILTRATION_API()
using MI_CHAMBER_FILTRATION = MI_SCREEN<N_("Chamber Filtration"), class ScreenChamberFiltration>;
#endif

#if HAS_MMU2()
/// MMU HW settings submenu
class MI_HW_MMU final : public IWindowMenuItem {
public:
    MI_HW_MMU();
    void click(IWindowMenu &) override;
};
#endif

#if HAS_E2EE_SUPPORT()
using MI_E2EE
    = MI_SCREEN<N_("Encryption"), class ScreenMenuE2ee, &img::padlock_16x16>;
#endif
