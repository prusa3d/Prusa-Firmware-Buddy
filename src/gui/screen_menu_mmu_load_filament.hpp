/**
 * @file screen_menu_mmu_load_filament.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

using ScreenMenuMMULoadFilament__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_LOAD_ALL, MI_MMU_LOAD_FILAMENT_1, MI_MMU_LOAD_FILAMENT_2, MI_MMU_LOAD_FILAMENT_3, MI_MMU_LOAD_FILAMENT_4, MI_MMU_LOAD_FILAMENT_5>;

class ScreenMenuMMULoadFilament : public ScreenMenuMMULoadFilament__ {
public:
    constexpr static const char *label = N_("Load Filament");
    ScreenMenuMMULoadFilament();
};
