/// @file
#pragma once

#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include <basic_screen_menu.hpp>

#include <option/has_touch.h>
#if HAS_TOUCH()
    #include "MItem_touch.hpp"
#endif

using ScreenMenuUserInterface__ = BasicScreenMenu<
    MI_FOOTER_SETTINGS,
    MI_SOUND_MODE,
    MI_SORT_FILES,
    MI_PRINT_PROGRESS_TIME,
    MI_TIMEOUT,
    MI_FILAMENT_CHANGE_PREHEAT_ALL,
#if HAS_ST7789_DISPLAY()
    // We could potentionally have MINI display without buzzer.
    // So we only allow sound control for ST7789
    MI_SOUND_VOLUME,
#endif
#if HAS_ILI9488_DISPLAY()
    MI_DISPLAY_BAUDRATE,
#endif
#if HAS_TOUCH()
    MI_ENABLE_TOUCH,
    TOUCH_SIG_WORKAROUND,
#endif
    MI_ALWAYS_HIDDEN>;

class ScreenMenuUserInterface final : public ScreenMenuUserInterface__ {
public:
    ScreenMenuUserInterface();
};
