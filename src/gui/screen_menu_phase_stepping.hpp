/// @file
#pragma once

#include <gui/basic_screen_menu.hpp>
#include <gui/menu_item/menu_item_gcode_action.hpp>
#include <option/has_phase_stepping.h>

static_assert(HAS_PHASE_STEPPING(), "Do not #include me if you are not using me");

class MI_PHASE_STEPPING_CALIBRATION final : public MenuItemGcodeAction {
public:
    MI_PHASE_STEPPING_CALIBRATION();
};

class MI_PHASE_STEPPING_RESTORE_DEFAULTS final : public MenuItemGcodeAction {
public:
    MI_PHASE_STEPPING_RESTORE_DEFAULTS();
};

using ScreenMenuPhaseSteppingBase = BasicScreenMenu<
    MI_PHASE_STEPPING_CALIBRATION,
    MI_PHASE_STEPPING_RESTORE_DEFAULTS>;

class ScreenMenuPhaseStepping final : public ScreenMenuPhaseSteppingBase {
public:
    ScreenMenuPhaseStepping();
};
