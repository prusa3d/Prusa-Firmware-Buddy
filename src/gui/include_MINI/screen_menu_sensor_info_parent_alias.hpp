/**
 * @file screen_menu_sensor_info_parent_alias.hpp
 * @brief alias for MINI printer info menu parent
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "MItem_MINI.hpp"

namespace detail {
using ScreenMenuSensorInfo = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_INFO_PRINTER_FILL_SENSOR, MI_MINDA, MI_INFO_MCU_TEMP>;
} // namespace detail
