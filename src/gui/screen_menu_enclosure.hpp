/**
 * @file screen_menu_enclosure.hpp
 * @brief displaying and setting of XL enclosure
 */

#pragma once

#include "screen_menu.hpp"
#include "MItem_enclosure.hpp"
#include "MItem_menus.hpp"
#include <MItem_tools.hpp>
#include "option/has_side_leds.h"
#include "option/has_toolchanger.h"

#include <device/board.h>
#include <gui/menu_item/specific/menu_items_chamber.hpp>

namespace detail {
using ScreenMenuEnclosure = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if XL_ENCLOSURE_SUPPORT()
    ,
    MI_ENCLOSURE_ENABLE, MI_CHAMBER_TEMP, MI_ENCLOSURE_FILTER_COUNTER, MI_ENCLOSURE_FILTER_CHANGE, MI_ENCLOSURE_MANUAL_SETTINGS
#endif
    >;

using ScreenMenuManualSetting = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if XL_ENCLOSURE_SUPPORT()
    ,
    MI_ENCLOSURE_PRINT_FILTRATION, MI_ENCLOSURE_POST_PRINT_FILTRATION, MI_ENCLOSURE_FAN_SETTING, MI_ENCLOSURE_POST_PRINT_DURATION
#endif
#if HAS_SIDE_LEDS()
    ,
    MI_SIDE_LEDS_ENABLE
#endif
#if HAS_TOOLCHANGER()
    ,
    MI_TOOL_LEDS_ENABLE,
    MI_TOOL_LEDS_BRIGHTNESS
#endif
    >;
} // namespace detail

class ScreenMenuEnclosure : public detail::ScreenMenuEnclosure {

public:
    constexpr static const char *label = N_("ENCLOSURE SETTINGS");
    ScreenMenuEnclosure();
};

class ScreenMenuManualSetting : public detail::ScreenMenuManualSetting {

public:
    constexpr static const char *label = N_("MANUAL SETTINGS");
    ScreenMenuManualSetting();
};
