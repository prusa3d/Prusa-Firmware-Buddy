/**
 * @file screen_menu_calibration_parent_alias.hpp
 * @brief parent alias of ScreenMenuCalibration for XL printer
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"
#include "MItem_basic_selftest.hpp"
#include "MItem_print.hpp"
#include <option/filament_sensor.h>
#include <option/has_side_fsensor.h>
#include <option/has_toolchanger.h>

using ScreenMenuCalibration__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_AUTO_HOME,
    MI_CALIB_Z, MI_SELFTEST_SNAKE, MI_CALIB_FSENSOR, MI_CALIB_FIRST, MI_LIVE_ADJUST_Z,
    MI_CALIBRATE_DOCK, MI_CALIBRATE_TOOL_OFFSETS,
    MI_RESTORE_CALIBRATION_FROM_USB, MI_BACKUP_CALIBRATION_TO_USB,
    MI_FS_SPAN<0, false>, MI_FS_SPAN<0, true>,
    MI_FS_SPAN<1, false>, MI_FS_SPAN<1, true>,
    MI_FS_SPAN<2, false>, MI_FS_SPAN<2, true>,
    MI_FS_SPAN<3, false>, MI_FS_SPAN<3, true>,
    MI_FS_SPAN<4, false>, MI_FS_SPAN<4, true>,
    MI_FS_SPAN<5, false>, MI_FS_SPAN<5, true>,
    MI_FS_REF<0, false>, MI_FS_REF<0, true>,
    MI_FS_REF<1, false>, MI_FS_REF<1, true>,
    MI_FS_REF<2, false>, MI_FS_REF<2, true>,
    MI_FS_REF<3, false>, MI_FS_REF<3, true>,
    MI_FS_REF<4, false>, MI_FS_REF<4, true>,
    MI_FS_REF<5, false>, MI_FS_REF<5, true>>;
