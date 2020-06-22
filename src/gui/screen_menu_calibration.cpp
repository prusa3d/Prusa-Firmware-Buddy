// screen_menu_calibration.cpp

#include "gui.h"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_print.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_WIZARD, MI_BABYSTEP, MI_AUTO_HOME, MI_MESH_BED,
    MI_SELFTEST, MI_CALIB_FIRST>;

static void init(screen_t *screen) {
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET));
    constexpr static const char *label = N_("Calibration");
    Screen::Create(screen, label);
}

screen_t screen_menu_calibration = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    nullptr,        //pdata
};

screen_t *const get_scr_menu_calibration() { return &screen_menu_calibration; }
