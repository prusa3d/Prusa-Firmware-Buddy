/// @file
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

using ScreenMenuMMUPreloadToMMU__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_PRELOAD_ALL, MI_MMU_PRELOAD_SLOT_1, MI_MMU_PRELOAD_SLOT_2, MI_MMU_PRELOAD_SLOT_3, MI_MMU_PRELOAD_SLOT_4, MI_MMU_PRELOAD_SLOT_5>;

class ScreenMenuMMUPreloadToMMU : public ScreenMenuMMUPreloadToMMU__ {
public:
    constexpr static const char *label = N_("Preload to MMU");
    ScreenMenuMMUPreloadToMMU();
};
