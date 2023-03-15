/**
 * @file screen_menu_control.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include <option/has_toolchanger.h>

using ScreenMenuControlSpec = ScreenMenu<EFooter::On, MI_RETURN,
#if HAS_TOOLCHANGER()
    MI_PICK_PARK_TOOL,
#endif
    MI_MOVE_AXIS, MI_TEMPERATURE, MI_SELFTEST_SNAKE, MI_AUTO_HOME, MI_DISABLE_STEP, MI_LIVE_ADJUST_Z>;

class ScreenMenuControl : public ScreenMenuControlSpec {
public:
    constexpr static const char *label = N_("CONTROL");
    ScreenMenuControl();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
