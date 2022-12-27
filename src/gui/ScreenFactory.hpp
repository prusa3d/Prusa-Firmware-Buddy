#pragma once

#include "screen_home.hpp"
#include "screen_splash.hpp"
#include "screen_menu_info.hpp"
#include "screen_menu_settings.hpp"
#include "screen_menu_tune.hpp"
#include "screen_menu_calibration.hpp"
#include "screen_menu_filament.hpp"
#include "screen_menu_temperature.hpp"
#include "screen_menu_move.hpp"
#include "screen_menu_sensor_info.hpp"
#include "screen_menu_odometer.hpp"
#include "screen_menu_version_info.hpp"
#include "screen_menu_fw_update.hpp"
#include "screen_menu_languages.hpp"
#include "screen_menu_lan_settings.hpp"
#include "screen_menu_hw_setup.hpp"
#include "screen_menu_steel_sheets.hpp"
#include "screen_menu_eeprom.hpp"
#include "screen_menu_footer_settings.hpp"
#include "screen_prusa_link.hpp"
#include "screen_menu_connect.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "screen_menu_network.hpp"
#include "screen_menu_eeprom_diagnostics.hpp"
#include "gui/test/screen_menu_test.hpp"

#include "static_alocation_ptr.hpp"
#include <array>

class ScreenFactory {
    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;
    using mem_space = std::aligned_union<0,
        ScreenMenuCalibration,
        ScreenMenuConnect,
        ScreenMenuEeprom,
        ScreenMenuEepromDiagnostics,
        ScreenMenuEthernetSettings,
        ScreenMenuExperimentalSettings,
        ScreenMenuFilament,
        ScreenMenuFooterSettings,
        ScreenMenuFooterSettingsAdv,
        ScreenMenuFwUpdate,
        ScreenMenuHwSetup,
        ScreenMenuInfo,
        ScreenMenuLanguages,
        ScreenMenuLanguagesNoRet,
        ScreenMenuMove,
        ScreenMenuNetwork,
        ScreenMenuOdometer,
        ScreenMenuSensorInfo,
        ScreenMenuSettings,
        ScreenMenuSteelSheets,
        ScreenMenuTemperature,
        ScreenMenuTest,
        ScreenMenuTune,
        ScreenMenuVersionInfo,
        ScreenMenuWifiSettings,
        ScreenMenuPrusaLink, screen_home_data_t, screen_splash_data_t>::type;

    static mem_space all_screens;

public:
    using UniquePtr = static_unique_ptr<screen_t>;
    using Creator = static_unique_ptr<screen_t> (*)(); //function pointer definition

    template <class T>
    static UniquePtr Screen() {
        static_assert(sizeof(T) <= sizeof(mem_space), "Screen memory space is too small");
        return make_static_unique_ptr<T>(&all_screens);
    }

    template <class T>
    static bool DoesCreatorHoldType(Creator cr) {
        return Screen<T> == cr;
    }
};
