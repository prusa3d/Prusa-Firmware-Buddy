// screen_menu_tune.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "screen_menus.hpp"
#include "marlin_client.h"
#include "MItem_print.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"

/*****************************************************************************/
//parent alias
using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_LIVE_ADJUST_Z, MI_M600, MI_SPEED, MI_NOZZLE,
    MI_HEATBED, MI_PRINTFAN, MI_FLOWFACT, MI_FILAMENT_SENSOR, MI_SOUND_MODE, MI_SOUND_VOLUME, MI_FAN_CHECK, MI_NETWORK, MI_TIMEZONE, MI_VERSION_INFO,
#ifdef _DEBUG
    MI_TEST,
#endif                       //_DEBUG
    /* MI_FOOTER_SETTINGS,*/ //currently experimental, but we want it in future
    MI_MESSAGES>;

class ScreenMenuTune : public Screen {
public:
    constexpr static const char *label = N_("TUNE");
    ScreenMenuTune()
        : Screen(_(label)) {
        Screen::ClrMenuTimeoutClose();
        //todo test if needed
        //marlin_update_vars(MARLIN_VAR_MSK_TEMP_TARG | MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT));
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

void ScreenMenuTune::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::LOOP:
        if (marlin_all_axes_homed() && marlin_all_axes_known() && (marlin_command() != MARLIN_CMD_G28) && (marlin_command() != MARLIN_CMD_G29) && (marlin_command() != MARLIN_CMD_M109) && (marlin_command() != MARLIN_CMD_M190)) {
            Item<MI_M600>().Enable();
        } else {
            Item<MI_M600>().Disable();
        }
        break;
    default:
        break;
    }
    SuperWindowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenMenuTune() {
    return ScreenFactory::Screen<ScreenMenuTune>();
}
