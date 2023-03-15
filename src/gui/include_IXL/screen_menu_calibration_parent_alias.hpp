/**
 * @file screen_menu_calibration_parent_alias.hpp
 * @brief parent alias of ScreenMenuCalibration for IXL printer
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_print.hpp"

using ScreenMenuCalibration__ = ScreenMenu<EFooter::On, MI_RETURN, MI_LIVE_ADJUST_Z, MI_AUTO_HOME, MI_MESH_BED>;
