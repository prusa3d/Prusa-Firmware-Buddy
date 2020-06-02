// screen_menu_temperature.c

#include "gui.h"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screens.h"
#include "menu_vars.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

#pragma pack(push, 1)

class MI_NOZZLE : public WI_SPIN_U16_t {
    constexpr static const char *label = "Nozzle";

public:
    MI_NOZZLE()
        : WI_SPIN_U16_t(uint16_t(marlin_vars()->target_nozzle),
            MenuVars::nozzle_range.data(), label, 0, true, false) {}
    virtual void OnClick() {
        marlin_set_target_nozzle(value);
    }
};

class MI_HEATBED : public WI_SPIN_U08_t {
    constexpr static const char *label = "Heatbed";

public:
    MI_HEATBED()
        : WI_SPIN_U08_t(uint8_t(marlin_vars()->target_bed),
            MenuVars::bed_range.data(), label, 0, true, false) {}
    virtual void OnClick() {
        marlin_set_target_bed(value);
    }
};

class MI_PRINTFAN : public WI_SPIN_U08_t {
    constexpr static const char *label = "Print Fan";

public:
    MI_PRINTFAN()
        : WI_SPIN_U08_t(uint8_t(marlin_vars()->fan_speed),
            MenuVars::printfan_range.data(), label, 0, true, false) {}
    virtual void OnClick() {
        marlin_set_fan_speed(value);
    }
};

class MI_COOLDOWN : public WI_LABEL_t {
    static constexpr const char *const label = "Cooldown";

public:
    MI_COOLDOWN()
        : WI_LABEL_t(label, 0, true, false) {
    }

protected:
    virtual void click(Iwindow_menu_t &window_menu) {
        screen_dispatch_event(NULL, WINDOW_EVENT_CLICK, (void *)this);
    }
};

/*****************************************************************************/
//parent alias
using parent = screen_menu_data_t<false, true, false, MI_RETURN, MI_NOZZLE, MI_HEATBED, MI_PRINTFAN, MI_COOLDOWN>;

class ScreenMenuTenperature : public parent {
public:
    constexpr static const char *label = "TEMPERATURE";
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};
#pragma pack(pop)

/*****************************************************************************/
//static member method definition
void ScreenMenuTenperature::Init(screen_t *screen) {
    marlin_update_vars(
        MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED));
    Create(screen, label);
}

int ScreenMenuTenperature::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuTenperature *const ths = reinterpret_cast<ScreenMenuTenperature *>(screen->pdata);
    if (event == WINDOW_EVENT_CLICK) {
        marlin_set_target_nozzle(0);
        marlin_set_target_bed(0);
        marlin_set_fan_speed(0);

        MI_NOZZLE *noz = reinterpret_cast<MI_NOZZLE *>(ths->menu.GetItem(1));
        MI_HEATBED *bed = reinterpret_cast<MI_HEATBED *>(ths->menu.GetItem(2));
        MI_PRINTFAN *fan = reinterpret_cast<MI_PRINTFAN *>(ths->menu.GetItem(3));
        noz->ClrVal();
        bed->ClrVal();
        fan->ClrVal();
    }

    return ths->Event(window, event, param);
}

screen_t screen_menu_temperature = {
    0,
    0,
    ScreenMenuTenperature::Init,
    ScreenMenuTenperature::CDone,
    ScreenMenuTenperature::CDraw,
    ScreenMenuTenperature::CEvent,
    sizeof(ScreenMenuTenperature), //data_size
    0,                             //pdata
};

extern "C" screen_t *const get_scr_menu_temperature() { return &screen_menu_temperature; }
