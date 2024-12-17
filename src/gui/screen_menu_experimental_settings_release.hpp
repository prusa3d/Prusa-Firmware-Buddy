/**
 * @file screen_menu_experimental_settings_release.hpp
 * @brief experimental settings for MINI printer
 * !! DO not include directly, include screen_menu_experimental_settings.hpp instead
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "MItem_experimental_tools.hpp"

#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

// parent alias
using ScreenMenuExperimentalSettings__ = ScreenMenu<GuiDefaults::MenuFooter, MI_SAVE_AND_RETURN,
#if PRINTER_IS_PRUSA_MK3_5()
    MI_ALT_FAN,
#endif
#if HAS_XBUDDY_EXTENSION()
    MI_XBE_FILTRATION_FAN,
#endif
    MI_Z_AXIS_LEN, MI_RESET_Z_AXIS_LEN, MI_STEPS_PER_UNIT_E, MI_RESET_STEPS_PER_UNIT, MI_DIRECTION_E, MI_RESET_DIRECTION, MI_SERIAL_PRINTING_SCREEN_ENABLE>;

struct ExperimentalSettingsValues {
    ExperimentalSettingsValues(ScreenMenuExperimentalSettings__ &parent);

    int32_t z_len;
    int32_t steps_per_unit_e; // has stored both index and polarity
    size_t touch_ena;

    // this is only safe as long as there are no gaps between variables
    // all variables are 32bit now, so it is safe
    bool operator==(const ExperimentalSettingsValues &other) const;
    bool operator!=(const ExperimentalSettingsValues &other) const;
};
