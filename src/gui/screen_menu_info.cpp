// screen_menu_info.c

#include "gui.h"
#include "screens.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"

#ifdef _DEBUG
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_STATISTIC_disabled, MI_SYS_INFO, MI_FAIL_STAT_disabled,
    MI_SUPPORT_disabled, MI_VERSION_INFO, MI_QR_info, MI_QR_test>;
#else
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_SYS_INFO, MI_VERSION_INFO>;
#endif //_DEBUG

static void init(screen_t *screen) {
    constexpr static const char *label = N_("INFO");
    Screen::Create(screen, label);
}

screen_t screen_menu_info = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    nullptr,        //pdata
};

screen_t *const get_scr_menu_info() { return &screen_menu_info; }
