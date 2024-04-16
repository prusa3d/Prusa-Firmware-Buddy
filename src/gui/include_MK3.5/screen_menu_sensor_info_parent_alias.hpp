/**
 * @file screen_menu_sensor_info_parent_alias.hpp
 * @brief alias for MK3.5 printer info menu parent
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "Configuration_adv.h"
#include "MItem_MK3.5.hpp"
#include "screen_menu_no_tools.hpp"
#include "MItem_mmu.hpp"

namespace detail {
using ScreenMenuSensorInfo = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if HAS_TEMP_BOARD
    ,
    MI_INFO_BOARD_TEMP
#endif

    ,
    MI_INFO_BED_TEMP,
    MI_INFO_MCU_TEMP,
    MI_INFO_NOZZLE_TEMP,
    MI_PINDA,
    MI_INFO_PRINTER_FILL_SENSOR,
    MI_INFO_FINDA,
    MI_INFO_PRINT_FAN,
    MI_INFO_HBR_FAN,
    MI_INFO_HEATER_VOLTAGE,
    MI_INFO_INPUT_VOLTAGE,
    MI_INFO_HEATER_CURRENT,
    MI_INFO_INPUT_CURRENT,
    MI_INFO_MMU_CURRENT>;
} // namespace detail
