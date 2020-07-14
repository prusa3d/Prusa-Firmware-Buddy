// screen_menu_temperature.c

#include "gui.hpp"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_print.hpp"

class MI_COOLDOWN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Cooldown");

public:
    MI_COOLDOWN()
        : WI_LABEL_t(label, 0, true, false) {
    }

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        screen_dispatch_event(nullptr, WINDOW_EVENT_CLICK, (void *)this);
    }
};

/*****************************************************************************/
//parent alias
using parent = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_NOZZLE, MI_HEATBED, MI_PRINTFAN, MI_COOLDOWN>;

class ScreenMenuTemperature : public parent {
public:
    constexpr static const char *label = N_("TEMPERATURE");
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};

/*****************************************************************************/
//static member method definition
void ScreenMenuTemperature::Init(screen_t *screen) {
    marlin_update_vars(
        MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED));
    Create(screen, _(label));
}

int ScreenMenuTemperature::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuTemperature *const ths = reinterpret_cast<ScreenMenuTemperature *>(screen->pdata);
    if (event == WINDOW_EVENT_CLICK) {
        marlin_set_target_nozzle(0);
        marlin_set_display_nozzle(0);
        marlin_set_target_bed(0);
        marlin_set_fan_speed(0);

        MI_NOZZLE *noz = &ths->Item<MI_NOZZLE>();
        MI_HEATBED *bed = &ths->Item<MI_HEATBED>();
        MI_PRINTFAN *fan = &ths->Item<MI_PRINTFAN>();
        noz->ClrVal();
        bed->ClrVal();
        fan->ClrVal();
    }

    return ths->Event(window, event, param);
}

screen_t screen_menu_temperature = {
    0,
    0,
    ScreenMenuTemperature::Init,
    ScreenMenuTemperature::CDone,
    ScreenMenuTemperature::CDraw,
    ScreenMenuTemperature::CEvent,
    sizeof(ScreenMenuTemperature), //data_size
    nullptr,                       //pdata
};

screen_t *const get_scr_menu_temperature() { return &screen_menu_temperature; }
