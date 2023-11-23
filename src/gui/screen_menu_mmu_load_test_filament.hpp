/**
 * @file screen_menu_mmu_load_filament.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

using ScreenMenuMMULoadTestFilament__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_LOAD_TEST_ALL, MI_MMU_LOAD_TEST_FILAMENT_1, MI_MMU_LOAD_TEST_FILAMENT_2, MI_MMU_LOAD_TEST_FILAMENT_3, MI_MMU_LOAD_TEST_FILAMENT_4, MI_MMU_LOAD_TEST_FILAMENT_5>;

class ScreenMenuMMULoadTestFilament : public ScreenMenuMMULoadTestFilament__ {
public:
    constexpr static const char *label = N_("Loading test");
    ScreenMenuMMULoadTestFilament();
};
