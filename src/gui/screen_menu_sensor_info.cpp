// screen_menu_sensors.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "ScreenHandler.hpp"
#include "MItem_tools.hpp"
#include "window_menu.hpp"
#include "DialogMoveZ.hpp"

#include "IWinMenuContainer.hpp"
#include <tuple>

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_FILAMENT_SENSOR_STATE, MI_MINDA>;

class ScreenMenuSensorInfo : public Screen {
protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("SENSOR INFO");
    ScreenMenuSensorInfo()
        : Screen(_(label)) {
        EnableLongHoldScreenAction();
        flags.timeout_close = is_closed_on_timeout_t::no;
    }
};

ScreenFactory::UniquePtr GetScreenMenuSensorInfo() {
    return ScreenFactory::Screen<ScreenMenuSensorInfo>();
}

void ScreenMenuSensorInfo::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (Item<MI_FILAMENT_SENSOR_STATE>().StateChanged())
            unconditionalDrawItem(1);
        if (Item<MI_MINDA>().StateChanged()) {
            unconditionalDrawItem(2);
        }
    }

    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    Screen::SuperWindowEvent(sender, event, param);
}
