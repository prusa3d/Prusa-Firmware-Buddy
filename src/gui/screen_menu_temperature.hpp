/**
 * @file screen_menu_temperature.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "MItem_filament.hpp"

/*****************************************************************************/
// parent alias
using ScreenMenuTemperature__ = ScreenMenu<
    EFooter::On, MI_RETURN, MI_NOZZLE<0>,
#if HAS_TOOLCHANGER()
    MI_NOZZLE<1>, MI_NOZZLE<2>, MI_NOZZLE<3>, MI_NOZZLE<4>,
#endif
    MI_HEATBED, MI_PRINTFAN, MI_COOLDOWN>;

class ScreenMenuTemperature : public ScreenMenuTemperature__ {
public:
    constexpr static const char *label = N_("TEMPERATURE");
    ScreenMenuTemperature();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
