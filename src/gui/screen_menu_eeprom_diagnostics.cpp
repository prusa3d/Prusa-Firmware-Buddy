/**
 * @file screen_menu_eeprom_diagnostics.cpp
 * @author Radek Vana
 * @brief Menu to show eeprom send/receive status
 * @date 2021-10-22
 */

#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "MItem_eeprom.hpp"

using Screen = ScreenMenu<EFooter::On, MI_RETURN,
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

class ScreenMenuEepromDiagnostics : public Screen {
public:
    constexpr static const char *label = N_("INFO");
    ScreenMenuEepromDiagnostics()
        : Screen(_(label)) {
    }
};

ScreenFactory::UniquePtr GetScreenMenuEepromDiagnostics() {
    return ScreenFactory::Screen<ScreenMenuEepromDiagnostics>();
}
