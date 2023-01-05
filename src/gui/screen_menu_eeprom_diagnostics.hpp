/**
 * @file screen_menu_eeprom_diagnostics.hpp
 * @brief Menu to show eeprom send/receive status
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_eeprom.hpp"

using ScreenMenuEepromDiagnostics__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_I2C_TRANSMIT_RESULTS_HAL_OK,
    MI_I2C_TRANSMIT_RESULTS_HAL_ERROR,
    MI_I2C_TRANSMIT_RESULTS_HAL_BUSY,
    MI_I2C_TRANSMIT_RESULTS_HAL_TIMEOUT,
    MI_I2C_TRANSMIT_RESULTS_UNDEF,

    MI_I2C_RECEIVE_RESULTS_HAL_OK,
    MI_I2C_RECEIVE_RESULTS_HAL_ERROR,
    MI_I2C_RECEIVE_RESULTS_HAL_BUSY,
    MI_I2C_RECEIVE_RESULTS_HAL_TIMEOUT,
    MI_I2C_RECEIVE_RESULTS_UNDEF>;

class ScreenMenuEepromDiagnostics : public ScreenMenuEepromDiagnostics__ {
public:
    constexpr static const char *label = N_("INFO");
    ScreenMenuEepromDiagnostics();
};
