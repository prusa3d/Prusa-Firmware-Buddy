// screen_menu_temperature.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"
#include "ScreenHandler.hpp"

class MI_COOLDOWN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Cooldown");

public:
    MI_COOLDOWN()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
    }

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        Screens::Access()->WindowEvent(GUI_event_t::CLICK, (void *)this);
    }
};

/*****************************************************************************/
//parent alias
using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_NOZZLE, MI_HEATBED, MI_PRINTFAN, MI_COOLDOWN>;

class ScreenMenuTemperature : public Screen {
public:
    constexpr static const char *label = N_("TEMPERATURE");
    ScreenMenuTemperature()
        : Screen(_(label)) {}

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

void ScreenMenuTemperature::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        marlin_set_target_nozzle(0);
        marlin_set_display_nozzle(0);
        marlin_set_target_bed(0);
        marlin_set_fan_speed(0);

        Item<MI_NOZZLE>().ClrVal();
        Item<MI_HEATBED>().ClrVal();
        Item<MI_PRINTFAN>().ClrVal();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

ScreenFactory::UniquePtr GetScreenMenuTemperature() {
    return ScreenFactory::Screen<ScreenMenuTemperature>();
}
