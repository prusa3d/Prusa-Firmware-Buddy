/**
 * @file screen_menu_sensor_info_parent_alias.hpp
 * @brief alias for XL printer info menu parent
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "Configuration_adv.h"
#include "MItem_loadcell.hpp"
#include "screen_menu_tools.hpp"
#include "screen_menu_modularbed.hpp"

namespace detail {
namespace internal {
    using ScreenMenuSensorInfo = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if (TEMP_SENSOR_HEATBREAK > 0)
        ,
        MI_INFO_HEATBREAK_N_TEMP<0>,
        MI_INFO_HEATBREAK_N_TEMP<1>,
        MI_INFO_HEATBREAK_N_TEMP<2>,
        MI_INFO_HEATBREAK_N_TEMP<3>,
        MI_INFO_HEATBREAK_N_TEMP<4>
#endif

#if HAS_TEMP_BOARD
        ,
        MI_INFO_BOARD_TEMP
#endif

        ,
        MI_INFO_BED_TEMP,
        MI_INFO_NOZZLE_N_TEMP<0>,
        MI_INFO_NOZZLE_N_TEMP<1>,
        MI_INFO_NOZZLE_N_TEMP<2>,
        MI_INFO_NOZZLE_N_TEMP<3>,
        MI_INFO_NOZZLE_N_TEMP<4>,
        MI_INFO_DWARF_BOARD_TEMPERATURE,
        MI_INFO_LOADCELL,
        MI_INFO_PRINTER_FILL_SENSOR,
        MI_FS_REF<0, false>,
        MI_FS_REF<1, false>,
        MI_FS_REF<2, false>,
        MI_FS_REF<3, false>,
        MI_FS_REF<4, false>,
        MI_FS_REF<5, false>,
        MI_INFO_SIDE_FILL_SENSOR,
        MI_FS_REF<0, true>,
        MI_FS_REF<1, true>,
        MI_FS_REF<2, true>,
        MI_FS_REF<3, true>,
        MI_FS_REF<4, true>,
        MI_FS_REF<5, true>,
        MI_INFO_PRINT_FAN,
        MI_INFO_HBR_FAN,
        MI_INFO_INPUT_VOLTAGE,
        MI_INFO_5V_VOLTAGE,
        MI_INFO_SANDWICH_5V_CURRENT,
        MI_INFO_BUDDY_5V_CURRENT>;
} // namespace internal
class ScreenMenuSensorInfo : public internal::ScreenMenuSensorInfo {
public:
    // recursive helper function for hiding inactive MI_FS_REF
    template <int N>
    void hide_fs_ref(uint8_t tool_nr) {
        if (tool_nr != N) {
            Hide<MI_FS_REF<N, false>>();
            Hide<MI_FS_REF<N, true>>();
        }
        if constexpr (N > 0) {
            hide_fs_ref<N - 1>(tool_nr);
        }
    }

    ScreenMenuSensorInfo(string_view_utf8 label, window_t *parent = nullptr)
        : internal::ScreenMenuSensorInfo(label, parent) {
        hide_fs_ref<5>(prusa_toolchanger.get_active_tool_nr());
    }
};
} // namespace detail
