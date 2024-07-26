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

namespace detail {
using ScreenMenuEnclosure = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN
#if XL_ENCLOSURE_SUPPORT()
    ,
    MI_ENCLOSURE_ENABLE, MI_ENCLOSURE_TEMP, MI_ENCLOSURE_FILTER_COUNTER, MI_ENCLOSURE_FILTER_CHANGE, MI_ENCLOSURE_MANUAL_SETTINGS
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
    MI_TOOL_LEDS_ENABLE
#endif
    >;
} // namespace detail

class ScreenMenuEnclosure : public detail::ScreenMenuEnclosure {

public:
    constexpr static const char *label = N_("ENCLOSURE SETTINGS");
    constexpr static const uint8_t loop_delay_s = 3;
    ScreenMenuEnclosure();

private:
    uint32_t last_ticks_s;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenMenuManualSetting : public detail::ScreenMenuManualSetting {

public:
    constexpr static const char *label = N_("MANUAL SETTINGS");
    ScreenMenuManualSetting();
};
