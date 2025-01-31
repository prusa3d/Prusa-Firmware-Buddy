/**
 * @file screen_menu_statistics.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "config_features.h"

using ScreenMenuStatistics__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
#if ANY(CRASH_RECOVERY, POWER_PANIC)
    MI_FAIL_STAT,
#endif // ANY(CRASH_RECOVERY, POWER_PANIC)
    MI_ODOMETER>;

class ScreenMenuStatistics : public ScreenMenuStatistics__ {
public:
    constexpr static const char *label = N_("PRINT STATISTICS");
    ScreenMenuStatistics();

private:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
    void updateUptime();
    IWindowMenuItem uptime_label;
};
