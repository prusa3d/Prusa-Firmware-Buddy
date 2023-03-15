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
#include "MItem_eeprom.hpp"
#include "MItem_filament.hpp"

using ScreenMenuFilamentMMU__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_MMU_LOAD_FILAMENT,
    MI_MMU_LOAD_TO_NOZZLE,
    MI_MMU_UNLOAD_FILAMENT, // @@TODO debug
    /*TODO MI_CHANGE,*/ MI_PURGE,
    MI_MMU_EJECT_FILAMENT,
    MI_MMU_CUT_FILAMENT,
    MI_MMU_SPOOLJOIN,
    MI_MMU_CUTTER,
    MI_MMU_FAIL_STATS,
    // @@TODO debug items, but the functionality will be used by the error screens
    MI_MMU_HOME0,
    MI_MMU_SW_RESET,
    MI_MMU_HW_RESET,
    MI_MMU_POWER_CYCLE>;

class ScreenMenuFilamentMMU : public ScreenMenuFilamentMMU__ {
public:
    constexpr static const char *label = N_("FILAMENT MMU");
    ScreenMenuFilamentMMU();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void deactivate_item();
};
