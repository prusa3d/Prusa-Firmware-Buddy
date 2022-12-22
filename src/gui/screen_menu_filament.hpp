/**
 * @file screen_menu_filament.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_filament.hpp"

using ScreenMenuFilament__ = ScreenMenu<EFooter::On, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE>;

class ScreenMenuFilament : public ScreenMenuFilament__ {
public:
    constexpr static const char *label = N_("FILAMENT");
    ScreenMenuFilament();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void deactivate_item();
};
