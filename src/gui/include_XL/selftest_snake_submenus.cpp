#include "selftest_snake_submenus.hpp"
#include <ScreenHandler.hpp>

namespace SelftestSnake {

void open_submenu(Action action) {
    if (action == Action::DockCalibration) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuDockCalibration>);
    } else if (action == Action::Loadcell) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuLoadcellTest>);
    } else if (action == Action::FilamentSensorCalibration) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilamentSensorsCalibration>);
    } else {
        assert(false && "Not a multitool action");
    }
}

ScreenMenuDockCalibration::ScreenMenuDockCalibration()
    : detail::ScreenMenuDockCalibration(_(label)) {}

void ScreenMenuDockCalibration::draw() {
    if (is_menu_draw_enabled()) {
        window_frame_t::draw();
    }
}

void ScreenMenuDockCalibration::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(sender, event, param, action, true);
}

ScreenMenuLoadcellTest::ScreenMenuLoadcellTest()
    : detail::ScreenMenuLoadcellTest(_(label)) {}

void ScreenMenuLoadcellTest::draw() {
    if (is_menu_draw_enabled()) {
        window_frame_t::draw();
    }
}

void ScreenMenuLoadcellTest::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(sender, event, param, action, true);
}

ScreenMenuFilamentSensorsCalibration::ScreenMenuFilamentSensorsCalibration()
    : detail::ScreenMenuFilamentSensorsCalibration(_(label)) {}

void ScreenMenuFilamentSensorsCalibration::draw() {
    if (is_menu_draw_enabled()) {
        window_frame_t::draw();
    }
}

void ScreenMenuFilamentSensorsCalibration::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(sender, event, param, action, true);
}

} // namespace SelftestSnake
