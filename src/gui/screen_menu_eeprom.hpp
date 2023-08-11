/**
 * @file screen_menu_eeprom.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"

/*****************************************************************************/
// parent alias
using ScreenMenuEeprom__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_EE_SAVEXML, MI_EE_CLEAR>;

class ScreenMenuEeprom : public ScreenMenuEeprom__ {
public:
    constexpr static const char *label = "Eeprom";
    ScreenMenuEeprom();
};
