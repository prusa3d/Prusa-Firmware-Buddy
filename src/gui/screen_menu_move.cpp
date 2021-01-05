// screen_menu_move.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
#include "menu_vars.h"
#include "WindowMenuItems.hpp"
#include "menu_spin_config.hpp"
#include "DialogHandler.hpp"

template <size_t INDEX>
class MI_AXIS : public WI_SPIN_I16_t {

public:
    MI_AXIS<INDEX>()
        : WI_SPIN_I16_t(int32_t(marlin_vars()->pos[INDEX]),
            SpinCnf::axis_ranges[INDEX], _(MenuVars::labels[INDEX]), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual invalidate_t Change(int dif) override {
        invalidate_t ret = WI_SPIN_I16_t::Change(dif);
        marlin_gcode_printf("G0 %c%d F%d", MenuVars::axis_letters[INDEX], value, MenuVars::manual_feedrate[INDEX]);
        return ret;
    }
};
/*
class MI_AXIS_E : public MI_AXIS<3> {
public:
    virtual void OnClick() override {
        marlin_gcode("G90");    // Set to Absolute Positioning
        marlin_gcode("M82");    // Set extruder to absolute mode
        marlin_gcode("G92 E0"); // Reset position before change
        ClrVal();               // Reset spin before change
        //original code erased invalid flag from menu. Why?
    }
};*/

class MI_AXIS_E : public WI_SPIN_I16_t {
public:
    MI_AXIS_E()
        : WI_SPIN_I16_t(0, SpinCnf::axis_ranges[3], _(MenuVars::labels[3]), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void OnClick() override {
        auto val = GetVal();
        if (val == 0)
            return;

        marlin_gcode("G90");    // Set to Absolute Positioning
        marlin_gcode("M82");    // Set extruder to absolute mode
        marlin_gcode("G92 E0"); // Reset position before change

        marlin_gcode_printf(val > 0 ? "M701 L0 P%d Z0" : "M702 U%d Z0", val);                                                     //positive value load, negative unload
        DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(val > 0 ? LoadUnloadMode::Load : LoadUnloadMode::Unload)); // open dialog now, do not wait for marlin thread
        ClrVal();                                                                                                                 // Reset spin before change
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

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

void ScreenMenuMove::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {

        bool temp_ok = (marlin_vars()->display_nozzle > MenuVars::extrude_min_temp);
        IWindowMenuItem *pAxis_E = &Item<MI_AXIS_E>();
        if (temp_ok && (!pAxis_E->IsEnabled()))
            pAxis_E->Enable();
        if ((!temp_ok) && pAxis_E->IsEnabled())
            pAxis_E->Disable();
    }

    SuperWindowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuMove() {
    return ScreenFactory::Screen<ScreenMenuMove>();
}
