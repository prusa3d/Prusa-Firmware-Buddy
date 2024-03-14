/**
 * @file screen_menu_mmu_cut_filament.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

using ScreenMenuMMUCutFilament__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_CUT_FILAMENT_1, MI_MMU_CUT_FILAMENT_2, MI_MMU_CUT_FILAMENT_3, MI_MMU_CUT_FILAMENT_4, MI_MMU_CUT_FILAMENT_5>;

class ScreenMenuMMUCutFilament : public ScreenMenuMMUCutFilament__ {
public:
    constexpr static const char *label = N_("Cut Filament");
    ScreenMenuMMUCutFilament();
};
