// screen_menu_sensors.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "ScreenHandler.hpp"
#include "MItem_tools.hpp"

using Screen = ScreenMenu<EHeader::On, EFooter::On, HelpLines_None, MI_RETURN, MI_MINDA, MI_FILAMENT_SENSOR_STATE>;

class ScreenMenuSensorInfo : public Screen {
public:
    constexpr static const char *label = N_("SENSOR INFO");
    ScreenMenuSensorInfo()
        : Screen(_(label)) {
    }
};

ScreenFactory::UniquePtr GetScreenMenuSensorInfo() {
    return ScreenFactory::Screen<ScreenMenuSensorInfo>();
}
