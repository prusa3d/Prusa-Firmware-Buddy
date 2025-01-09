/// @file
#pragma once

#include "menu_item/menu_item_gcode_action.hpp"
#include "screen_menu.hpp"
#include <option/has_phase_stepping.h>
#include <option/has_phase_stepping_toggle.h>

static_assert(HAS_PHASE_STEPPING(), "Do not #include me if you are not using me");

using MI_PHASE_STEPPING_CALIBRATION = WithConstructorArgs<
    MenuItemGcodeAction,
    N_("Calibration"), "M1977"_tstr>;

using MI_PHASE_STEPPING_RESTORE_DEFAULTS = WithConstructorArgs<
    MenuItemGcodeAction,
    N_("Restore Defaults"), "M1977 D"_tstr>;

using ScreenMenuPhaseSteppingBase = ScreenMenu<
    GuiDefaults::MenuFooter,
    MI_RETURN,
#if HAS_PHASE_STEPPING_TOGGLE()
    MI_PHASE_STEPPING_TOGGLE,
#endif
    MI_PHASE_STEPPING_CALIBRATION,
    MI_PHASE_STEPPING_RESTORE_DEFAULTS>;

class ScreenMenuPhaseStepping final : public ScreenMenuPhaseSteppingBase {
public:
    ScreenMenuPhaseStepping()
        : ScreenMenuPhaseSteppingBase(_("PHASE STEPPING")) {
    }
};
