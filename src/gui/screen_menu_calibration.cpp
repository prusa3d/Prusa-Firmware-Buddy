// screen_menu_calibration.cpp

#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_print.hpp"
#include "printers.h"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_WIZARD, MI_LIVE_ADJUST_Z, MI_AUTO_HOME, MI_MESH_BED,
    MI_SELFTEST, MI_CALIB_FIRST, MI_TEST_FANS, MI_TEST_XYZ, MI_TEST_HEAT
#ifdef _DEBUG
    ,
    MI_ADVANCED_FAN_TEST, MI_TEST_ABORT
#endif
    >;
/*
static void init(screen_t *screen) {
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET));
    constexpr static const char *label = N_("CALIBRATION");
    Screen::Create(screen, _(label));
}
*/
class ScreenMenuCalibration : public Screen {
public:
    constexpr static const char *label = N_("CALIBRATION");
    ScreenMenuCalibration()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuCalibration() {
    return ScreenFactory::Screen<ScreenMenuCalibration>();
}
