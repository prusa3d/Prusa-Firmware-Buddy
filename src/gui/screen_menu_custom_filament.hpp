/**
 * @file screen_menu_custom_filament.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "WindowMenuItems.hpp"
#include <option/has_toolchanger.h>

using ScreenMenuCustomFilament__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_CUSTOM_FILAMENT_SLOT, MI_CUSTOM_FILAMENT_NOZZLE_TEMP, MI_CUSTOM_FILAMENT_NOZZLE_PREHEAT_TEMP, MI_CUSTOM_FILAMENT_HEATBED_TEMP>;

class ScreenMenuCustomFilament : public ScreenMenuCustomFilament__ {
public:
    constexpr static const char *label = N_("CUSTOM FILAMENT");
    ScreenMenuCustomFilament();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
