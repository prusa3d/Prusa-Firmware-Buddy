/**
 * @file menu_items_open.cpp
 */

#include "menu_items_open.hpp"
#include "ScreenHandler.hpp"
#include "ScreenFactory.hpp"
#include "png_resources.hpp"
#include "screen_sysinf.hpp"

#include "screen_messages.hpp"
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
#include "screen_menu_eeprom.hpp"
#include "screen_menu_footer_settings.hpp"
#include "screen_prusa_link.hpp"
#include "screen_menu_connect.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "screen_menu_network.hpp"
#include "screen_menu_eeprom_diagnostics.hpp"
#include "gui/test/screen_menu_test.hpp"

#define GENERATE_OPENER_ITEM_DEFINITION__(NAME, SCREEN, ICON, VISIBILITY) \
    NAME::NAME()                                                          \
        : WI_LABEL_t(_(label), ICON, is_enabled_t::yes, VISIBILITY) {     \
    }                                                                     \
    void NAME::click(IWindowMenu &) {                                     \
        Screens::Access()->Open(ScreenFactory::Screen<SCREEN>);           \
    }

#define GENERATE_OPENER_ITEM_DEFINITION_ICON(NAME, SCREEN, ICON) GENERATE_OPENER_ITEM_DEFINITION__(NAME, SCREEN, ICON, is_hidden_t::no)
#define GENERATE_OPENER_ITEM_DEFINITION(NAME, SCREEN)            GENERATE_OPENER_ITEM_DEFINITION__(NAME, SCREEN, nullptr, is_hidden_t::no)
#define GENERATE_OPENER_ITEM_DEFINITION_DEV(NAME, SCREEN)        GENERATE_OPENER_ITEM_DEFINITION__(NAME, SCREEN, nullptr, is_hidden_t::dev)

#define GENERATE_OPENER_ITEM_DEFINITION_DISABLED(NAME)                       \
    NAME::NAME()                                                             \
        : WI_LABEL_t(_(label), nullptr, is_enabled_t::no, is_hidden_t::no) { \
    }                                                                        \
    void NAME::click(IWindowMenu &) {                                        \
    }

GENERATE_OPENER_ITEM_DEFINITION(MI_VERSION_INFO, ScreenMenuVersionInfo);
GENERATE_OPENER_ITEM_DEFINITION(MI_SENSOR_INFO, ScreenMenuSensorInfo);
GENERATE_OPENER_ITEM_DEFINITION(MI_ODOMETER, ScreenMenuOdometer);
GENERATE_OPENER_ITEM_DEFINITION(MI_FILAMENT, ScreenMenuFilament);
GENERATE_OPENER_ITEM_DEFINITION(MI_SYS_INFO, screen_sysinfo_data_t);
GENERATE_OPENER_ITEM_DEFINITION(MI_TEMPERATURE, ScreenMenuTemperature);
GENERATE_OPENER_ITEM_DEFINITION(MI_MOVE_AXIS, ScreenMenuMove);
GENERATE_OPENER_ITEM_DEFINITION(MI_TEST, ScreenMenuTest);
GENERATE_OPENER_ITEM_DEFINITION(MI_FW_UPDATE, ScreenMenuFwUpdate);
GENERATE_OPENER_ITEM_DEFINITION(MI_MESSAGES, screen_messages_data_t);
GENERATE_OPENER_ITEM_DEFINITION(MI_LANGUAGE, ScreenMenuLanguages);
GENERATE_OPENER_ITEM_DEFINITION(MI_HW_SETUP, ScreenMenuHwSetup);
GENERATE_OPENER_ITEM_DEFINITION(MI_EEPROM, ScreenMenuEeprom);
GENERATE_OPENER_ITEM_DEFINITION(MI_FOOTER_SETTINGS, ScreenMenuFooterSettings);
GENERATE_OPENER_ITEM_DEFINITION(MI_FOOTER_SETTINGS_ADV, ScreenMenuFooterSettingsAdv);
GENERATE_OPENER_ITEM_DEFINITION(MI_PRUSALINK, ScreenMenuPrusaLink);
GENERATE_OPENER_ITEM_DEFINITION(MI_PRUSA_CONNECT, ScreenMenuConnect);
GENERATE_OPENER_ITEM_DEFINITION(MI_NETWORK, ScreenMenuNetwork);
GENERATE_OPENER_ITEM_DEFINITION(MI_EXPERIMENTAL_SETTINGS, ScreenMenuExperimentalSettings);

GENERATE_OPENER_ITEM_DEFINITION_DEV(MI_EEPROM_DIAGNOSTICS, ScreenMenuEepromDiagnostics);

GENERATE_OPENER_ITEM_DEFINITION_ICON(MI_ETH_SETTINGS, ScreenMenuEthernetSettings, &png::lan_16x16);
GENERATE_OPENER_ITEM_DEFINITION_ICON(MI_WIFI_SETTINGS, ScreenMenuWifiSettings, &png::wifi_16x16);

GENERATE_OPENER_ITEM_DEFINITION_DISABLED(MI_FAIL_STAT_disabled);
GENERATE_OPENER_ITEM_DEFINITION_DISABLED(MI_SUPPORT_disabled);

/*****************************************************************************/
//MI_SERVICE
MI_SERVICE::MI_SERVICE()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SERVICE::click(IWindowMenu & /*window_menu*/) {
    //screen_open(get_scr_menu_service()->id);
}
