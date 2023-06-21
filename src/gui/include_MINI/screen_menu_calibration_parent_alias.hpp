/**
 * @file screen_menu_calibration_parent_alias.hpp
 * @brief parent alias of ScreenMenuCalibration for MINI printer
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_basic_selftest.hpp"
#include "MItem_print.hpp"

using ScreenMenuCalibration__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_WIZARD, MI_LIVE_ADJUST_Z, MI_AUTO_HOME, MI_MESH_BED,
    MI_SELFTEST, MI_CALIB_FIRST, MI_TEST_FANS, MI_TEST_XYZ, MI_TEST_HEAT, MI_SELFTEST_RESULT>;
