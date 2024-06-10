/**
 * @file screen_menu_preheat.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_preheat.hpp"
#include "MItem_filament.hpp"

using ScreenMenuPreheat__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_PREHEAT, MI_PREHEAT_NOZZLE, MI_PREHEAT_BED, MI_PREHEAT_COOLDOWN>;

class ScreenMenuPreheat : public ScreenMenuPreheat__ {
public:
    constexpr static const char *label = N_("PREHEAT");
    ScreenMenuPreheat();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
