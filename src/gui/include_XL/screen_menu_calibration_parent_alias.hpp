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
    MI_CALIBRATE_KENNEL, MI_CALIBRATE_TOOL_OFFSETS,
    MI_RESTORE_CALIBRATION_FROM_USB, MI_BACKUP_CALIBRATION_TO_USB,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_0>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_0>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_1>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_1>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_2>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_2>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_3>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_3>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_4>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_4>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_5>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_5>>;
