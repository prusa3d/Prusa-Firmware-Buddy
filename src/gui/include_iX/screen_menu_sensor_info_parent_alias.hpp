/**
 * @file screen_menu_sensor_info_parent_alias.hpp
 * @brief alias for iX printer info menu parent
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "Configuration_adv.h"
#include "MItem_love_board.hpp"
#include "MItem_loadcell.hpp"
#include "screen_menu_no_tools.hpp"
#include "screen_menu_modularbed.hpp"

namespace detail {
using ScreenMenuSensorInfo = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if (TEMP_SENSOR_HEATBREAK > 0)
    ,
    MI_INFO_HEATBREAK_TEMP
#endif

#if HAS_TEMP_BOARD
    ,
    MI_INFO_BOARD_TEMP
#endif

    ,
    MI_INFO_BED_TEMP,
    MI_INFO_MCU_TEMP,
    MI_INFO_NOZZLE_TEMP,
    MI_INFO_LOADCELL,
    MI_INFO_PRINTER_FILL_SENSOR,
    MI_INFO_PRINT_FAN,
    MI_INFO_HBR_FAN,
    MI_INFO_HEATER_VOLTAGE,
    MI_INFO_INPUT_VOLTAGE,
    MI_INFO_HEATER_CURRENT,
    MI_INFO_INPUT_CURRENT,
    MI_INFO_MMU_CURRENT>;
} // namespace detail
