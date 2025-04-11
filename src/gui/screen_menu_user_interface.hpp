/**
 * @file screen_menu_user_interface.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"
#include "printers.h"
#include <option/has_side_leds.h>
#include <option/has_toolchanger.h>
#include <option/has_leds.h>
#include <option/has_touch.h>
#include <option/has_xbuddy_extension.h>

#if HAS_TOUCH()
    #include "MItem_touch.hpp"
#endif

#if HAS_XBUDDY_EXTENSION()
    #include <menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

using ScreenMenuUserInterface__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_FOOTER_SETTINGS, MI_SORT_FILES,
#if not PRINTER_IS_PRUSA_MINI()
    MI_PRINT_PROGRESS_TIME,
#endif
    MI_TIMEOUT, MI_SOUND_MODE, MI_HEATUP_BED,
#if HAS_ST7789_DISPLAY()
    // We could potentionally have MINI display without buzzer.
    // So we only allow sound control for ST7789
    MI_SOUND_VOLUME,
#endif
#if HAS_LEDS()
    MI_LEDS_ENABLE,
    MI_DISPLAY_BACKLIGHT_BRIGHTNESS,
#endif
#if HAS_SIDE_LEDS()
    MI_SIDE_LEDS_ENABLE,
    MI_SIDE_LEDS_DIMMING,
#endif
#if HAS_TOOLCHANGER()
    MI_TOOL_LEDS_ENABLE,
    MI_TOOL_LEDS_BRIGHTNESS,
#endif /*HAS_TOOLCHANGER()*/
#if HAS_TOUCH()
    MI_ENABLE_TOUCH, TOUCH_SIG_WORKAROUND, MI_TOUCH_PLAYGROUND,
#endif
    MI_ALWAYS_HIDDEN>;

class ScreenMenuUserInterface : public ScreenMenuUserInterface__ {
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
    int last_touch_error_count = 0;

public:
    constexpr static const char *label = N_("USER INTERFACE");
    ScreenMenuUserInterface();
};
