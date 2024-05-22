#pragma once
#include <screen_menu_selftest_snake.hpp>

namespace SelftestSnake {

template <Tool tool>
using MI_STS_Dock_Calib = MI_STS_SUBMENU<tool, Action::DockCalibration>;

template <Tool tool>
using MI_STS_LoadcellTest = MI_STS_SUBMENU<tool, Action::Loadcell>;

template <Tool tool>
using MI_STS_FSensor_Calibration = MI_STS_SUBMENU<tool, Action::FilamentSensorCalibration>;

namespace detail {
    using ScreenMenuDockCalibration = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
        MI_STS_Dock_Calib<Tool::Tool1>, MI_STS_Dock_Calib<Tool::Tool2>, MI_STS_Dock_Calib<Tool::Tool3>, MI_STS_Dock_Calib<Tool::Tool4>, MI_STS_Dock_Calib<Tool::Tool5>>;

    using ScreenMenuLoadcellTest = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
        MI_STS_LoadcellTest<Tool::Tool1>, MI_STS_LoadcellTest<Tool::Tool2>, MI_STS_LoadcellTest<Tool::Tool3>, MI_STS_LoadcellTest<Tool::Tool4>, MI_STS_LoadcellTest<Tool::Tool5>>;

    using ScreenMenuFilamentSensorsCalibration = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
        MI_STS_FSensor_Calibration<Tool::Tool1>, MI_STS_FSensor_Calibration<Tool::Tool2>, MI_STS_FSensor_Calibration<Tool::Tool3>, MI_STS_FSensor_Calibration<Tool::Tool4>, MI_STS_FSensor_Calibration<Tool::Tool5>>;
} // namespace detail

class ScreenMenuDockCalibration : public detail::ScreenMenuDockCalibration {
public:
    static constexpr const char *label { "Dock Calibration" };
    static constexpr Action action { Action::DockCalibration };
    ScreenMenuDockCalibration();

    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenMenuLoadcellTest : public detail::ScreenMenuLoadcellTest {
public:
    static constexpr const char *label { "Loadcell Test" };
    static constexpr Action action { Action::Loadcell };
    ScreenMenuLoadcellTest();

    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenMenuFilamentSensorsCalibration : public detail::ScreenMenuFilamentSensorsCalibration {
public:
    static constexpr const char *label { "Filament Sensor Calibration" };
    static constexpr Action action { Action::FilamentSensorCalibration };
    ScreenMenuFilamentSensorsCalibration();

    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

void open_submenu(Action action);
} // namespace SelftestSnake
