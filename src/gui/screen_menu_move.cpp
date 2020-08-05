// screen_menu_move.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
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

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_AXIS_X, MI_AXIS_Y, MI_AXIS_Z, MI_AXIS_E>;

class ScreenMenuMove : public Screen {
public:
    constexpr static const char *label = N_("MOVE AXIS");
    ScreenMenuMove()
        : Screen(_(label)) {}
    virtual void windowEvent(window_t *sender, uint8_t ev, void *param) override;
};

void ScreenMenuMove::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_LOOP) {

        bool temp_ok = (marlin_vars()->target_nozzle > MenuVars::extrude_min_temp);
        IWindowMenuItem *pAxis_E = &Item<MI_AXIS_E>();
        if (temp_ok && (!pAxis_E->IsEnabled()))
            pAxis_E->Enable();
        if ((!temp_ok) && pAxis_E->IsEnabled())
            pAxis_E->Disable();
    }

    Screen::windowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuMove() {
    return ScreenFactory::Screen<ScreenMenuMove>();
}
