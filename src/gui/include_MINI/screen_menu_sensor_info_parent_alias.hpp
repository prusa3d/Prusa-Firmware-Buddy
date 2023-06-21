/**
 * @file screen_menu_sensor_info_parent_alias.hpp
 * @brief alias for MINI printer info menu parent
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "MItem_MINI_MK3.5.hpp"

namespace detail {
using ScreenMenuSensorInfo = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_FILAMENT_SENSOR_STATE, MI_MINDA>;
} // namespace detail
