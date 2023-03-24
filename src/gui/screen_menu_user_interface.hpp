/**
 * @file screen_menu_user_interface.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_touch.hpp"
#include "MItem_menus.hpp"
#include "printers.h"
#include <option/has_side_leds.h>

using ScreenMenuUserInterfaceInSettings__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_FOOTER_SETTINGS, MI_SORT_FILES, MI_PRINT_PROGRESS_TIME, MI_TIMEOUT, MI_SOUND_MODE, MI_HEATUP_BED
#if (PRINTER_TYPE != PRINTER_PRUSA_XL && PRINTER_TYPE != PRINTER_PRUSA_MK4)
    ,
    MI_SOUND_VOLUME
#endif
#if HAS_LEDS
    ,
    MI_LEDS_ENABLE
#endif
#if HAS_SIDE_LEDS()
    ,
    MI_SIDE_LEDS_ENABLE
#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_XL) || (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    ,
    MI_ENABLE_TOUCH
#endif
    >;

class ScreenMenuUserInterfaceInSettings : public ScreenMenuUserInterfaceInSettings__ {
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("USER INTERFACE");
    ScreenMenuUserInterfaceInSettings();
};

class ScreenMenuUserInterfaceInTune : public ScreenMenuUserInterfaceInSettings {
public:
    ScreenMenuUserInterfaceInTune();
};
