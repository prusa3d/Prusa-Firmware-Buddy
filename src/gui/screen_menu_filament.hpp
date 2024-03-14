/**
 * @file screen_menu_filament.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_filament.hpp"
#include <option/has_toolchanger.h>

using ScreenMenuFilament__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE
#if HAS_TOOLCHANGER()
    ,
    MI_CHANGEALL
#endif /*HAS_TOOLCHANGER()*/
    >;

class ScreenMenuFilament : public ScreenMenuFilament__ {
public:
    constexpr static const char *label = N_("FILAMENT");
    ScreenMenuFilament();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void deactivate_item();
};
