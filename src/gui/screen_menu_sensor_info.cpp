// screen_menu_sensors.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "ScreenHandler.hpp"
#include "MItem_tools.hpp"

using Screen = ScreenMenu<EHeader::On, EFooter::On, HelpLines_None, MI_RETURN, MI_MINDA, MI_FILAMENT_SENSOR_STATE>;

class ScreenMenuSensorInfo : public Screen {
private:
    int32_t last_refresh = 0;

public:
    constexpr static const char *label = N_("SENSOR INFO");
    ScreenMenuSensorInfo()
        : Screen(_(label)) {
    }
    virtual void windowEvent(window_t *sender, uint8_t ev, void *param) override;
};

ScreenFactory::UniquePtr GetScreenMenuSensorInfo() {
    return ScreenFactory::Screen<ScreenMenuSensorInfo>();
}

void ScreenMenuSensorInfo::windowEvent(window_t *sender, uint8_t ev, void *param) {
    if (ev == WINDOW_EVENT_LOOP) {
        Item<MI_FILAMENT_SENSOR_STATE>().CheckValue();
        if (HAL_GetTick() - last_refresh > 100) {
            Invalidate();
            last_refresh = HAL_GetTick();
        }
    }

    Screen::windowEvent(sender, ev, param);
}
