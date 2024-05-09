#pragma once

#include "screen_home.hpp"
#include "screen_splash.hpp"
#include "screen_menu_info.hpp"
#include "screen_menu_settings.hpp"
#include "screen_menu_tune.hpp"
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
#include "screen_menu_fail_stat.hpp"
#include "screen_menu_diagnostics.hpp"
#include "screen_menu_user_interface.hpp"
#include "screen_menu_lang_and_time.hpp"
#include "screen_menu_hardware.hpp"
#include "screen_menu_hardware_checks.hpp"
#include "screen_menu_system.hpp"
#include "screen_menu_statistics.hpp"
#include "screen_menu_cancel_object.hpp"
#include "screen_touch_error.hpp"
#include "screen_print_preview.hpp"
#include "screen_menu_input_shaper.hpp"
#include "screen_menu_enclosure.hpp"
#include "screen_printing.hpp"

#if HAS_COLDPULL()
    #include "screen_cold_pull.hpp"
#endif

#include "gui/test/screen_menu_test.hpp"

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "screen_menu_mmu_preload_to_mmu.hpp"
    #include "screen_menu_mmu_eject_filament.hpp"
    #include "screen_menu_mmu_cut_filament.hpp"
    #include "screen_menu_mmu_load_to_nozzle.hpp"
    #include "screen_menu_filament_mmu.hpp"
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "screen_menu_tools.hpp"
#endif

#include "static_alocation_ptr.hpp"
#include <array>
#include "config.h"

class ScreenFactory {
    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;
    using mem_space = std::aligned_union<0,
        ScreenMenuCancelObject,
        ScreenMenuConnect,
        ScreenMenuDiagnostics,
        ScreenMenuEeprom,
        ScreenMenuEepromDiagnostics,
        ScreenMenuEthernetSettings,
        ScreenMenuExperimentalSettings,
        ScreenMenuFailStat,
        ScreenMenuFilament,
        ScreenMenuFooterSettings,
        ScreenMenuFooterSettingsAdv,
        ScreenMenuFwUpdate,
        ScreenMenuHardware,
        ScreenMenuHardwareChecks,
        ScreenMenuHwSetup,
        ScreenMenuInfo,
        ScreenMenuLangAndTime,
        ScreenMenuLanguages,
        ScreenMenuLanguagesNoRet,
        ScreenMenuMove,
        ScreenMenuNetwork,
        ScreenMenuOdometer,
        ScreenMenuSensorInfo,
        ScreenMenuSettings,
        ScreenMenuStatistics,
        ScreenMenuSteelSheets,
        ScreenMenuSystem,
        ScreenMenuTemperature,
        ScreenMenuTest,
        ScreenTouchError,
        ScreenMenuTune,
        ScreenMenuUserInterface,
        ScreenMenuVersionInfo,
        ScreenMenuWifiSettings,
        ScreenPrintPreview,
        ScreenMenuPrusaLink,
        ScreenMenuInputShaper,
        ScreenMenuEnclosure,
        ScreenMenuManualSetting,
#if HAS_MMU2()
        ScreenMenuFilamentMMU,
        ScreenMenuMMUCutFilament,
        ScreenMenuMMUEjectFilament,
        ScreenMenuMMUPreloadToMMU,
        ScreenMenuMMULoadToNozzle,
#endif
#if HAS_COLDPULL()
        ScreenColdPull,
#endif
        screen_home_data_t, screen_splash_data_t, screen_printing_data_t>::type;

    static mem_space all_screens;

public:
    using UniquePtr = static_unique_ptr<screen_t>;
    using Creator = static_unique_ptr<screen_t> (*)(); // function pointer definition

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
