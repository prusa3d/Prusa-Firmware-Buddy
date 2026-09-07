/**
 * @file screen_menu_hardware_tune.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "MItem_crash.hpp"
#include <option/has_crash_detection.h>

using ScreenMenuHardwareTune__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if HAS_CRASH_DETECTION()
    ,
    MI_CRASH_SENSITIVITY_X, MI_CRASH_MAX_PERIOD_X, MI_CRASH_SENSITIVITY_Y, MI_CRASH_MAX_PERIOD_Y
    #if HAS_DRIVER(TMC2130)
    ,
    MI_CRASH_FILTERING
    #endif
#endif
    >;

class ScreenMenuHardwareTune : public ScreenMenuHardwareTune__ {
public:
    constexpr static const char *label = N_("HARDWARE");
    ScreenMenuHardwareTune();
};
