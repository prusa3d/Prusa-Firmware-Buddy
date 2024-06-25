/**
 * @file screen_menu_info.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"

using ScreenMenuInfo__ = ScreenMenu<EFooter::On, MI_RETURN,
#ifdef _DEBUG
    MI_SYS_INFO,
#endif //_DEBUG
    MI_NETWORK_STATUS,
    MI_SENSOR_INFO, MI_VERSION_INFO, MI_PRINT_STATISTICS>;

class ScreenMenuInfo : public ScreenMenuInfo__ {
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("INFO");
    ScreenMenuInfo();
};
