/**
 * @file screen_menu_hardware_tune.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "MItem_crash.hpp"

using ScreenMenuHardwareTune__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if ENABLED(CRASH_RECOVERY)
    ,
    MI_CRASH_SENSITIVITY_X, MI_CRASH_MAX_PERIOD_X, MI_CRASH_SENSITIVITY_Y, MI_CRASH_MAX_PERIOD_Y
    #if HAS_DRIVER(TMC2130)
    ,
    MI_CRASH_FILTERING
    #endif
#endif // ENABLED(CRASH_RECOVERY)
    >;

class ScreenMenuHardwareTune : public ScreenMenuHardwareTune__ {
public:
    constexpr static const char *label = N_("HARDWARE");
    ScreenMenuHardwareTune();
};
