/**
 * @file screen_menu_lang_and_time.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"

using ScreenMenuLangAndTime__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
#if HAS_TRANSLATIONS()
    MI_LANGUAGE,
#endif
    MI_TIMEZONE, MI_TIMEZONE_MIN, MI_TIMEZONE_SUMMER, MI_TIME_FORMAT
#if PRINTER_IS_PRUSA_MINI()
    ,
    MI_TIME_NOW // Mini does not show time in header, so show it here
#endif
    >;

class ScreenMenuLangAndTime : public ScreenMenuLangAndTime__ {
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("LANGUAGE & TIME");
    ScreenMenuLangAndTime();
};
