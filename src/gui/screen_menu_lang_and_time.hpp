/**
 * @file screen_menu_lang_and_time.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"
#include "menu_items_languages.hpp"

using ScreenMenuLangAndTime__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_LANGUAGE, MI_TIMEZONE, MI_TIME_FORMAT, MI_LANGUAGUE_USB, MI_LOAD_LANG, MI_LANGUAGUE_XFLASH>;

class ScreenMenuLangAndTime : public ScreenMenuLangAndTime__ {
public:
    constexpr static const char *label = N_("LANGUAGE & TIME");
    ScreenMenuLangAndTime();
};
