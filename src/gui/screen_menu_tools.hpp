/**
 * @file screen_menu_tools.hpp
 * @brief This is temporary menu enabling kennel position and tool offset view and edit. Simple manual calibration of the kennel position is included.
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "selftest_frame.hpp"
#include "MItem_hardware.hpp"

class MI_TOOL_NOZZLE_DIAMETER : public MI_NOZZLE_DIAMETER {
public:
    MI_TOOL_NOZZLE_DIAMETER();
};

class MI_POSITION : public WiSpinFlt {
public:
    MI_POSITION(string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, float initVal);

protected:
    virtual void set_pos(const float pos) = 0;

    virtual void OnClick() override;
};

class MI_KENNEL_POSITION_X : public MI_POSITION {
    static constexpr const char *label = N_("Kennel X");
    static constexpr const char *axis_name = "X";

public:
    MI_KENNEL_POSITION_X();

    virtual void set_pos(const float pos) override;
};

class MI_KENNEL_POSITION_Y : public MI_POSITION {
    static constexpr const char *label = N_("Kennel Y");
    static constexpr const char *axis_name = "Y";

public:
    MI_KENNEL_POSITION_Y();

    virtual void set_pos(const float pos) override;
};

class MI_OFFSET : public WiSpinFlt {
public:
    MI_OFFSET(string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, float initVal);
};

class MI_OFFSET_X : public MI_OFFSET {
    static constexpr const char *label = N_("Offset X");

public:
    MI_OFFSET_X();

    virtual void OnClick() override;
};

class MI_OFFSET_Y : public MI_OFFSET {
    static constexpr const char *label = N_("Offset Y");

public:
    MI_OFFSET_Y();

    virtual void OnClick() override;
};

class MI_OFFSET_Z : public MI_OFFSET {
    static constexpr const char *label = N_("Offset Z");

public:
    MI_OFFSET_Z();

    virtual void OnClick() override;
};

class MI_PICKUP_TOOL : public WI_LABEL_t {
    static constexpr const char *label = N_("Pickup Tool");

public:
    MI_PICKUP_TOOL();

    virtual void click(IWindowMenu &window_menu) override;
};

class MI_KENNEL_CALIBRATE : public WI_LABEL_t {
    static constexpr const char *label = N_("Calibrate Kennel Position");

public:
    MI_KENNEL_CALIBRATE();

    virtual void click(IWindowMenu &window_menu) override;
};

class MI_FSENSORS_CALIBRATE : public WI_LABEL_t {
    static constexpr const char *label = N_("Calibrate Filament Sensor");

public:
    MI_FSENSORS_CALIBRATE();

    virtual void click(IWindowMenu &window_menu) override;
};

namespace detail {
using ScreenMenuToolSetup = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_TOOL_NOZZLE_DIAMETER, MI_KENNEL_POSITION_X, MI_KENNEL_POSITION_Y, MI_KENNEL_CALIBRATE, MI_FSENSORS_CALIBRATE, MI_OFFSET_X, MI_OFFSET_Y, MI_OFFSET_Z, MI_PICKUP_TOOL>;
}

class ScreenMenuToolSetup : public detail::ScreenMenuToolSetup {
public:
    ScreenMenuToolSetup();

    constexpr static const char *labels[] = { N_("TOOL 1"), N_("TOOL 2"), N_("TOOL 3"), N_("TOOL 4"), N_("TOOL 5") };
};

class I_MI_TOOL : public WI_LABEL_t {

public:
    I_MI_TOOL(const char *const label, int index);

protected:
    void do_click(int index);
};

template <int N>
class MI_TOOL : public I_MI_TOOL {
    static constexpr const char *const get_name() {
        switch (N) {
        case 0:
            return N_("Tool 1");
        case 1:
            return N_("Tool 2");
        case 2:
            return N_("Tool 3");
        case 3:
            return N_("Tool 4");
        case 4:
            return N_("Tool 5");
        }
    }

    static constexpr const char *const label = get_name();

public:
    MI_TOOL()
        : I_MI_TOOL(label, N) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        do_click(N);
    }
};

class MI_PARK_TOOL : public WI_LABEL_t {
    static constexpr const char *label = N_("Park Current Tool");

public:
    MI_PARK_TOOL();

    virtual void click(IWindowMenu &window_menu) override;
};

namespace detail {
using ScreenMenuTools = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_TOOL<0>, MI_TOOL<1>, MI_TOOL<2>, MI_TOOL<3>, MI_TOOL<4>, MI_PARK_TOOL>;
}

class ScreenMenuTools : public detail::ScreenMenuTools {
public:
    constexpr static const char *label = N_("TOOLS");
    ScreenMenuTools();
};
