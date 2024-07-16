/**
 * @file screen_menu_filament_mmu.hpp
 * @brief filament menu for MMU
 * it will not open if MMU is disabled, so no need to check it
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "menu_item_event_dispatcher.hpp"
#include "MItem_mmu.hpp"
#include "MItem_filament.hpp"
#include <gui/screen/filament/screen_filament_management.hpp>

using ScreenMenuFilamentMMU__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_PRELOAD,
    MI_MMU_PRELOAD_ADVANCED,
    MI_MMU_LOAD_TO_NOZZLE,
    MI_MMU_UNLOAD_FILAMENT,
    MI_MMU_EJECT_FILAMENT,
    MI_MMU_CUT_FILAMENT,
    // MI_PURGE, doing purges in MMU mode is not only unsupported but dangerous and doesn't play well with the preferred workflow of the MMU-enabled machine
    // debug items, but the functionality will be used by the error screens - hidden in release builds
    MI_MMU_SW_RESET,
    MI_MMU_HW_RESET,
    MI_MMU_POWER_CYCLE,
    MI_FILAMENT_MANAGEMENT>;

class ScreenMenuFilamentMMU : public ScreenMenuFilamentMMU__ {
public:
    constexpr static const char *label = N_("FILAMENT MMU");
    ScreenMenuFilamentMMU();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
