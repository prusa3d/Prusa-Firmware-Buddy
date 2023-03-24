#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "MItem_basic_selftest.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif
#include <utility_extensions.hpp>

namespace SelftestSnake {
enum class Tool {
    Tool1 = 0,
    Tool2 = 1,
    Tool3 = 2,
    Tool4 = 3,
    Tool5 = 4,
    _count,
    _all_tools = _count,
    _last = _count - 1,
    _first = Tool1,
};

// Order matters, snake will be run in the same order, text indices are also dependant on this.
// Keep in mind screen menu items need to be in the same order.
enum class Action {
    Fans,
    ZAlign, // also known as z_calib
    XYCheck,
    KennelCalibration,
    Loadcell,
    ToolOffsetsCalibration,
    ZCheck,
    Heaters,
    FilamentSensorCalibration,
    _count,
    _last = _count - 1,
    _first = Fans,
};
static_assert(Action::_first != Action::_last, "Edge case not handled");

template <Action action>
concept SubmenuActionC = action == Action::KennelCalibration || action == Action::Loadcell || action == Action::FilamentSensorCalibration;

constexpr bool is_multitool_only_action(Action action) {
    return action == Action::KennelCalibration || action == Action::ToolOffsetsCalibration;
}

consteval auto get_submenu_label(Tool tool, Action action) -> const char * {
    struct ToolText {
        Tool tool;
        Action action;
        const char *label;
    };
    const ToolText tooltexts[] {
        { Tool::Tool1, Action::KennelCalibration, N_("Kennel 1 Calibration") },
        { Tool::Tool2, Action::KennelCalibration, N_("Kennel 2 Calibration") },
        { Tool::Tool3, Action::KennelCalibration, N_("Kennel 3 Calibration") },
        { Tool::Tool4, Action::KennelCalibration, N_("Kennel 4 Calibration") },
        { Tool::Tool5, Action::KennelCalibration, N_("Kennel 5 Calibration") },
        { Tool::Tool1, Action::Loadcell, N_("Tool 1 Loadcell Test") },
        { Tool::Tool2, Action::Loadcell, N_("Tool 2 Loadcell Test") },
        { Tool::Tool3, Action::Loadcell, N_("Tool 3 Loadcell Test") },
        { Tool::Tool4, Action::Loadcell, N_("Tool 4 Loadcell Test") },
        { Tool::Tool5, Action::Loadcell, N_("Tool 5 Loadcell Test") },
        { Tool::Tool1, Action::FilamentSensorCalibration, N_("Tool 1 Filament Sensor Calibration") },
        { Tool::Tool2, Action::FilamentSensorCalibration, N_("Tool 2 Filament Sensor Calibration") },
        { Tool::Tool3, Action::FilamentSensorCalibration, N_("Tool 3 Filament Sensor Calibration") },
        { Tool::Tool4, Action::FilamentSensorCalibration, N_("Tool 4 Filament Sensor Calibration") },
        { Tool::Tool5, Action::FilamentSensorCalibration, N_("Tool 5 Filament Sensor Calibration") },
    };

    if (auto it = std::ranges::find_if(tooltexts, [&](const auto &elem) {
            return elem.tool == tool && elem.action == action;
        });
        it != std::end(tooltexts)) {
        return it->label;
    } else {
        consteval_assert_false("Unable to find a label for this combination");
        return "";
    }
}

class I_MI_STS : public WI_LABEL_t {
public:
    static constexpr size_t max_label_len { 35 };
    I_MI_STS(Action action);
    void do_click(IWindowMenu &window_menu, Action action);

private:
    char *get_filled_menu_item_label(Action action);
    char label_buffer[max_label_len];
};

template <Action action>
class MI_STS : public I_MI_STS {
public:
    MI_STS()
        : I_MI_STS(action) {}

protected:
    void click(IWindowMenu &window_menu) override {
        do_click(window_menu, action);
    }
};

class I_MI_STS_SUBMENU : public WI_LABEL_t {
public:
    I_MI_STS_SUBMENU(const char *label, Action action, Tool tool);
    void do_click(IWindowMenu &window_menu, Tool tool, Action action);
};

template <Tool tool, Action action>
requires SubmenuActionC<action> class MI_STS_SUBMENU : public I_MI_STS_SUBMENU {
public:
    MI_STS_SUBMENU()
        : I_MI_STS_SUBMENU(get_submenu_label(tool, action), action, tool) {}

protected:
    void click(IWindowMenu &window_menu) override {
        do_click(window_menu, tool, action);
    }
};

template <Tool tool>
using MI_STS_Kennel_Calib = MI_STS_SUBMENU<tool, Action::KennelCalibration>;

template <Tool tool>
using MI_STS_LoadcellTest = MI_STS_SUBMENU<tool, Action::Loadcell>;

template <Tool tool>
using MI_STS_FSensor_Calibration = MI_STS_SUBMENU<tool, Action::FilamentSensorCalibration>;

namespace detail {
    using ScreenMenuSelftestSnake = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_STS<Action::Fans>, MI_STS<Action::ZAlign>, MI_STS<Action::XYCheck>,
#if HAS_TOOLCHANGER()
        MI_STS<Action::KennelCalibration>,
#endif
        MI_STS<Action::Loadcell>,
#if HAS_TOOLCHANGER()
        MI_STS<Action::ToolOffsetsCalibration>,
#endif
        MI_STS<Action::ZCheck>, MI_STS<Action::Heaters>, MI_STS<Action::FilamentSensorCalibration>>;

    using ScreenMenuSTSWizard = ScreenMenu<GuiDefaults::MenuFooter,
        MI_STS<Action::Fans>, MI_STS<Action::ZAlign>, MI_STS<Action::XYCheck>,
#if HAS_TOOLCHANGER()
        MI_STS<Action::KennelCalibration>,
#endif
        MI_STS<Action::Loadcell>,
#if HAS_TOOLCHANGER()
        MI_STS<Action::ToolOffsetsCalibration>,
#endif
        MI_STS<Action::ZCheck>,
        MI_STS<Action::Heaters>, MI_STS<Action::FilamentSensorCalibration>, MI_EXIT>;

    using ScreenMenuKennelCalibration = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
        MI_STS_Kennel_Calib<Tool::Tool1>, MI_STS_Kennel_Calib<Tool::Tool2>, MI_STS_Kennel_Calib<Tool::Tool3>, MI_STS_Kennel_Calib<Tool::Tool4>, MI_STS_Kennel_Calib<Tool::Tool5>>;
    using ScreenMenuLoadcellTest = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
        MI_STS_LoadcellTest<Tool::Tool1>, MI_STS_LoadcellTest<Tool::Tool2>, MI_STS_LoadcellTest<Tool::Tool3>, MI_STS_LoadcellTest<Tool::Tool4>, MI_STS_LoadcellTest<Tool::Tool5>>;
    using ScreenMenuFilamentSensorsCalibration = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
        MI_STS_FSensor_Calibration<Tool::Tool1>, MI_STS_FSensor_Calibration<Tool::Tool2>, MI_STS_FSensor_Calibration<Tool::Tool3>, MI_STS_FSensor_Calibration<Tool::Tool4>, MI_STS_FSensor_Calibration<Tool::Tool5>>;
}

class ScreenMenuKennelCalibration : public detail::ScreenMenuKennelCalibration {
public:
    static constexpr const char *label { "Kennel Calibration" };
    static constexpr Action action { Action::KennelCalibration };
    ScreenMenuKennelCalibration();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenMenuLoadcellTest : public detail::ScreenMenuLoadcellTest {
public:
    static constexpr const char *label { "Loadcell Test" };
    static constexpr Action action { Action::Loadcell };
    ScreenMenuLoadcellTest();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenMenuFilamentSensorsCalibration : public detail::ScreenMenuFilamentSensorsCalibration {
public:
    static constexpr const char *label { "Filament Sensor Calibration" };
    static constexpr Action action { Action::FilamentSensorCalibration };
    ScreenMenuFilamentSensorsCalibration();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
} // namespace SelftestSnake

class ScreenMenuSelftestSnake : public SelftestSnake::detail::ScreenMenuSelftestSnake {
public:
    static constexpr const char *label { "CALIBRATIONS & TESTS" };
    ScreenMenuSelftestSnake();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenMenuSTSWizard : public SelftestSnake::detail::ScreenMenuSTSWizard {
public:
    static constexpr const char *label { "Wizard" };
    ScreenMenuSTSWizard();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
};
