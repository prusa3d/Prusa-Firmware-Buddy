/**
 * @file screen_menu_mmu_load_to_nozzle.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

using ScreenMenuMMULoadToNozzle__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_LOAD_TO_NOZZLE_1, MI_MMU_LOAD_TO_NOZZLE_2, MI_MMU_LOAD_TO_NOZZLE_3, MI_MMU_LOAD_TO_NOZZLE_4, MI_MMU_LOAD_TO_NOZZLE_5>;

class ScreenMenuMMULoadToNozzle : public ScreenMenuMMULoadToNozzle__ {
public:
    constexpr static const char *label = N_("Load to Nozzle");
    ScreenMenuMMULoadToNozzle();
};
