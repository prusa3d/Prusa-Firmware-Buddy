/**
 * @file screen_menu_fail_stat.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_crash.hpp"

using ScreenMenuFailStat__ = ScreenMenu<EFooter::On, MI_RETURN
#if ENABLED(POWER_PANIC)
    ,
    MI_POWER_PANICS /*filament runout,*/
#endif              // ENABLED(POWER_PANIC)
#if ENABLED(CRASH_RECOVERY)
    ,
    MI_CRASHES_X_LAST, MI_CRASHES_Y_LAST, MI_CRASHES_X, MI_CRASHES_Y
#endif // ENABLED(CRASH_RECOVERY)
    >;

class ScreenMenuFailStat : public ScreenMenuFailStat__ {
    static constexpr const char *label = N_("FAILURE STATISTICS");

public:
    ScreenMenuFailStat();
};
