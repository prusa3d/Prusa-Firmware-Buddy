/**
 * @file screen_menu_control.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include <option/has_toolchanger.h>
#include <printers.h>
#include "MItem_basic_selftest.hpp"

using ScreenMenuControlSpec = ScreenMenu<EFooter::On, MI_RETURN,
#if HAS_TOOLCHANGER()
    MI_PICK_PARK_TOOL,
#endif
    MI_MOVE_AXIS,
    MI_TEMPERATURE,
    MI_AUTO_HOME,
    MI_DISABLE_STEP,
    MI_LIVE_ADJUST_Z,
#if (PRINTER_TYPE == PRINTER_PRUSA_XL)
    MI_SELFTEST_SNAKE
#else
    MI_CALIB_FIRST,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_0>,
    MI_CALIB_Z,
    MI_CALIB_FSENSOR,
    MI_CALIB_FSENSOR_MMU,
    MI_SELFTEST,
    MI_DIAGNOSTICS
#endif
    >;

class ScreenMenuControl : public ScreenMenuControlSpec {
public:
    constexpr static const char *label = N_("CONTROL");
    ScreenMenuControl();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
