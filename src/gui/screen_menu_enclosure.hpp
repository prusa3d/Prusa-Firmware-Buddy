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
#include <option/has_chamber_filtration_api.h>
#include <device/board.h>
#include <gui/menu_item/specific/menu_items_chamber.hpp>
#include <option/has_lights_menu.h>

namespace detail {
using ScreenMenuEnclosure = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if XL_ENCLOSURE_SUPPORT()
    ,
    MI_ENCLOSURE_ENABLE
#endif
#if HAS_CHAMBER_FILTRATION_API()
    ,
    MI_CHAMBER_FILTRATION
#endif
#if HAS_CHAMBER_API()
    ,
    MI_CHAMBER_TEMP
#endif
#if HAS_LIGHTS_MENU()
    ,
    MI_LIGHTS
#endif
    >;
} // namespace detail

class ScreenMenuEnclosure : public detail::ScreenMenuEnclosure {

public:
    constexpr static const char *label = N_("ENCLOSURE SETTINGS");
    ScreenMenuEnclosure();
};
