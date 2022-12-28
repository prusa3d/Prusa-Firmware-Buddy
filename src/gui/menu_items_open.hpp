/**
 * @file menu_items_open.hpp
 * @brief Screen opening menu items
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

#define GENERATE_OPENER_ITEM_DECLARATION(NAME, LABEL)          \
    class NAME : public WI_LABEL_t {                           \
        static constexpr const char *const label = LABEL;      \
                                                               \
    public:                                                    \
        NAME();                                                \
                                                               \
    protected:                                                 \
        virtual void click(IWindowMenu &window_menu) override; \
    }

GENERATE_OPENER_ITEM_DECLARATION(MI_VERSION_INFO, N_("Version Info"));
GENERATE_OPENER_ITEM_DECLARATION(MI_SENSOR_INFO, N_("Sensor Info"));
GENERATE_OPENER_ITEM_DECLARATION(MI_ODOMETER, N_("Version Info"));
GENERATE_OPENER_ITEM_DECLARATION(MI_FILAMENT, N_("Statistics"));
GENERATE_OPENER_ITEM_DECLARATION(MI_SYS_INFO, N_("Filament"));
GENERATE_OPENER_ITEM_DECLARATION(MI_TEMPERATURE, N_("Version Info"));
GENERATE_OPENER_ITEM_DECLARATION(MI_MOVE_AXIS, N_("Move Axis"));
GENERATE_OPENER_ITEM_DECLARATION(MI_SERVICE, N_("Service"));
GENERATE_OPENER_ITEM_DECLARATION(MI_TEST, N_("Test"));
GENERATE_OPENER_ITEM_DECLARATION(MI_FW_UPDATE, N_("FW Update"));
GENERATE_OPENER_ITEM_DECLARATION(MI_ETH_SETTINGS, N_("Ethernet"));
GENERATE_OPENER_ITEM_DECLARATION(MI_WIFI_SETTINGS, N_("Wi-Fi"));
GENERATE_OPENER_ITEM_DECLARATION(MI_MESSAGES, N_("Messages"));
GENERATE_OPENER_ITEM_DECLARATION(MI_LANGUAGE, N_("Language"));
GENERATE_OPENER_ITEM_DECLARATION(MI_PRUSA_CONNECT, N_("Prusa Connect"));
GENERATE_OPENER_ITEM_DECLARATION(MI_HW_SETUP, N_("HW Setup"));
GENERATE_OPENER_ITEM_DECLARATION(MI_EEPROM, N_("Eeprom"));
GENERATE_OPENER_ITEM_DECLARATION(MI_FOOTER_SETTINGS, N_("Footer Settings"));
GENERATE_OPENER_ITEM_DECLARATION(MI_FOOTER_SETTINGS_ADV, N_("Advanced"));
GENERATE_OPENER_ITEM_DECLARATION(MI_EXPERIMENTAL_SETTINGS, N_("Experimental Settings"));
GENERATE_OPENER_ITEM_DECLARATION(MI_EEPROM_DIAGNOSTICS, "Eeprom Diagnostics");
GENERATE_OPENER_ITEM_DECLARATION(MI_PRUSALINK, "PrusaLink");
GENERATE_OPENER_ITEM_DECLARATION(MI_NETWORK, N_("Network"));
GENERATE_OPENER_ITEM_DECLARATION(MI_FAIL_STAT_disabled, N_("Temperature"));
GENERATE_OPENER_ITEM_DECLARATION(MI_SUPPORT_disabled, N_("Support"));
