/**
 * @file screen_menu_calibration_parent_alias.hpp
 * @brief parent alias of ScreenMenuCalibration MK4 printer
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"
#include "MItem_basic_selftest.hpp"
#include "MItem_print.hpp"
#include <option/filament_sensor.h>
#include <option/has_toolchanger.h>

using ScreenMenuCalibration__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_SELFTEST, MI_AUTO_HOME,
    MI_CALIB_Z, MI_DIAGNOSTICS, MI_CALIB_FIRST, MI_LIVE_ADJUST_Z, MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_0>, MI_CALIB_FSENSOR>;
