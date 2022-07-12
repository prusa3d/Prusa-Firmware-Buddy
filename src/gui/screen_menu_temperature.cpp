// screen_menu_temperature.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "MItem_filament.hpp"
#include "ScreenHandler.hpp"

/*****************************************************************************/
//parent alias
using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_NOZZLE, MI_HEATBED, MI_PRINTFAN, MI_COOLDOWN>;

class ScreenMenuTemperature : public Screen {
public:
    constexpr static const char *label = N_("TEMPERATURE");
    ScreenMenuTemperature()
        : Screen(_(label)) {
        EnableLongHoldScreenAction();
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

void ScreenMenuTemperature::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        marlin_set_target_nozzle(0);
        marlin_set_display_nozzle(0);
        marlin_set_target_bed(0);
        marlin_set_fan_speed(0);

        Item<MI_NOZZLE>().SetVal(0);
        Item<MI_HEATBED>().SetVal(0);
        Item<MI_PRINTFAN>().SetVal(0);
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

ScreenFactory::UniquePtr GetScreenMenuTemperature() {
    return ScreenFactory::Screen<ScreenMenuTemperature>();
}
