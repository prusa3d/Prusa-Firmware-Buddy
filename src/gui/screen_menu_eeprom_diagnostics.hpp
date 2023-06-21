/**
 * @file screen_menu_eeprom_diagnostics.hpp
 * @brief Menu to show eeprom send/receive status
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_eeprom.hpp"

using ScreenMenuEepromDiagnostics__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_EEPROM_INIT_CRC_ERROR,
    MI_EEPROM_INIT_UPGRADED,
    MI_EEPROM_INIT_UPGRADE_FAILED,

    MI_I2C_TRANSMIT_RESULTS_HAL_OK,
    MI_I2C_TRANSMIT_RESULTS_HAL_BUSY,

    MI_I2C_RECEIVE_RESULTS_HAL_OK,
    MI_I2C_RECEIVE_RESULTS_HAL_BUSY>;

class ScreenMenuEepromDiagnostics : public ScreenMenuEepromDiagnostics__ {
public:
    constexpr static const char *label = N_("INFO");
    ScreenMenuEepromDiagnostics();
};
