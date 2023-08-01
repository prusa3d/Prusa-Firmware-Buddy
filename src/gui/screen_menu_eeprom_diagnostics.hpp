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

    MI_I2C_RESULTS_HAL_OK<1>,
    MI_I2C_RESULTS_HAL_ERROR<1>,
    MI_I2C_RESULTS_HAL_BUSY<1>,
    MI_I2C_RESULTS_HAL_TIMEOUT<1>,

    MI_I2C_RESULTS_HAL_OK<2>,
    MI_I2C_RESULTS_HAL_ERROR<2>,
    MI_I2C_RESULTS_HAL_BUSY<2>,
    MI_I2C_RESULTS_HAL_TIMEOUT<2>,

    MI_I2C_RESULTS_HAL_OK<3>,
    MI_I2C_RESULTS_HAL_ERROR<3>,
    MI_I2C_RESULTS_HAL_BUSY<3>,
    MI_I2C_RESULTS_HAL_TIMEOUT<3>>;

class ScreenMenuEepromDiagnostics : public ScreenMenuEepromDiagnostics__ {
public:
    constexpr static const char *label = N_("INFO");
    ScreenMenuEepromDiagnostics();
};
