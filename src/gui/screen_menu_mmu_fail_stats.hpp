/**
 * @file screen_menu_mmu_fail_stats.hpp
 */

#pragma once
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

using ScreenMenuMMUFailStats__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN>;

class ScreenMenuMMUFailStats : public ScreenMenuMMUFailStats__ {
public:
    constexpr static const char *label = N_("MMU Fail Stats");
    ScreenMenuMMUFailStats();
};
