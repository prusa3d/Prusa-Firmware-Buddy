/// @file
#pragma once

#include <basic_screen_menu.hpp>
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"

using ScreenMenuLangAndTimeBase = BasicScreenMenu<
#if HAS_TRANSLATIONS()
    MI_LANGUAGE,
#endif
    MI_TIMEZONE,
    MI_TIMEZONE_MIN,
    MI_TIMEZONE_SUMMER,
    MI_TIME_FORMAT
#if PRINTER_IS_PRUSA_MINI()
    ,
    MI_TIME_NOW // Mini does not show time in header, so show it here
#endif
    >;

class ScreenMenuLangAndTime final : public ScreenMenuLangAndTimeBase {
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    ScreenMenuLangAndTime();
};
