/**
 * @file screen_menu_mmu_eject_filament.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

using ScreenMenuMMUEjectFilament__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_EJECT_FILAMENT_1, MI_MMU_EJECT_FILAMENT_2, MI_MMU_EJECT_FILAMENT_3, MI_MMU_EJECT_FILAMENT_4, MI_MMU_EJECT_FILAMENT_5>;

class ScreenMenuMMUEjectFilament : public ScreenMenuMMUEjectFilament__ {
public:
    constexpr static const char *label = N_("Eject Filament");
    ScreenMenuMMUEjectFilament();
};
