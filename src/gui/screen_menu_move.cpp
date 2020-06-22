// screen_menu_move.cpp

#include "gui.h"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screens.h"
#include "menu_vars.h"
#include "WindowMenuItems.hpp"

template <size_t INDEX>
class MI_AXIS : public WI_SPIN_I16_t {

public:
    MI_AXIS<INDEX>()
        : WI_SPIN_I16_t(int32_t(marlin_vars()->pos[INDEX]),
            MenuVars::axis_ranges[INDEX].data(), MenuVars::labels[INDEX], 0, true, false) {}
    virtual bool Change(int dif) override {
        bool ret = WI_SPIN_I16_t::Change(dif);
        marlin_gcode_printf("G0 %c%d F%d", MenuVars::axis_letters[INDEX], value, MenuVars::manual_feedrate[INDEX]);
        return ret;
    }
};

class MI_AXIS_E : public MI_AXIS<3> {
public:
    virtual void OnClick() override {
        marlin_gcode("G90");    // Set to Absolute Positioning
        marlin_gcode("M82");    // Set extruder to absolute mode
        marlin_gcode("G92 E0"); // Reset position before change
        value = 0;              // Reset spin before change
        //original code erased invalid flag from menu. Why?
    }
};

using MI_AXIS_X = MI_AXIS<0>;
using MI_AXIS_Y = MI_AXIS<1>;
using MI_AXIS_Z = MI_AXIS<2>;

using parent = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_AXIS_X, MI_AXIS_Y, MI_AXIS_Z, MI_AXIS_E>;

class ScreenMenuMove : public parent {
public:
    constexpr static const char *label = N_("Settings");
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};

/*****************************************************************************/
//static member method definition
void ScreenMenuMove::Init(screen_t *screen) {
    marlin_update_vars(MARLIN_VAR_MSK_POS_XYZE | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ));
    Create(screen, label);
}

int ScreenMenuMove::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuMove *const ths = reinterpret_cast<ScreenMenuMove *>(screen->pdata);
    if (event == WINDOW_EVENT_LOOP) {

        bool temp_ok = (marlin_vars()->target_nozzle > MenuVars::extrude_min_temp);
        IWindowMenuItem *pAxis_E = &ths->Item<MI_AXIS_E>();
        if (temp_ok && (!pAxis_E->IsEnabled()))
            pAxis_E->Enable();
        if ((!temp_ok) && pAxis_E->IsEnabled())
            pAxis_E->Disable();
    }

    return ths->Event(window, event, param);
}

screen_t screen_menu_move = {
    0,
    0,
    ScreenMenuMove::Init,
    ScreenMenuMove::CDone,
    ScreenMenuMove::CDraw,
    ScreenMenuMove::CEvent,
    sizeof(ScreenMenuMove), //data_size
    nullptr,                //pdata
};

screen_t *const get_scr_menu_move() { return &screen_menu_move; }
