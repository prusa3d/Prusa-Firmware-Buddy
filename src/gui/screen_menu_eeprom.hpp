/**
 * @file screen_menu_eeprom.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"

/*****************************************************************************/
//parent alias
using ScreenMenuEeprom__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_EE_LOAD_400, MI_EE_LOAD_401,
    MI_EE_LOAD_402, MI_EE_LOAD_403RC1, MI_EE_LOAD_403,
    MI_EE_LOAD, MI_EE_SAVE, MI_EE_SAVEXML, MI_EE_CLEAR>;

class ScreenMenuEeprom : public ScreenMenuEeprom__ {
public:
    constexpr static const char *label = "Eeprom";
    ScreenMenuEeprom();
};
