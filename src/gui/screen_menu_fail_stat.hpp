/**
 * @file screen_menu_fail_stat.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_crash.hpp"
#include "MItem_mmu.hpp"
#include "MItem_menus.hpp"
#include <option/has_crash_detection.h>
#include <option/has_mmu2.h>
#include <option/has_indx.h>
#include <option/has_power_panic.h>

#if HAS_INDX()
    #include "MItem_tools.hpp"
#endif

using ScreenMenuFailStat__ = ScreenMenu<EFooter::On, MI_RETURN
#if HAS_POWER_PANIC()
    ,
    MI_POWER_PANICS /*filament runout,*/
#endif
#if HAS_CRASH_DETECTION()
    ,
    MI_CRASHES_X_LAST, MI_CRASHES_Y_LAST, MI_CRASHES_X, MI_CRASHES_Y
#endif
#if HAS_MMU2()
    ,
    MI_MMU_LOAD_FAILS, MI_MMU_TOTAL_LOAD_FAILS, MI_MMU_GENERAL_FAILS, MI_MMU_TOTAL_GENERAL_FAILS
#endif
#if HAS_INDX()
    ,
    MI_INFO_INDX_PICKUP_FAIL, MI_INFO_INDX_PARK_FAIL
#endif
    >;

class ScreenMenuFailStat : public ScreenMenuFailStat__ {
    static constexpr const char *label = N_("FAILURE STATISTICS");

public:
    ScreenMenuFailStat();
};
