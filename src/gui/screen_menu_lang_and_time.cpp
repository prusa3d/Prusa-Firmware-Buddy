/**
 * @file screen_menu_lang_and_time.cpp
 */

#include "screen_menu_lang_and_time.hpp"

ScreenMenuLangAndTime::ScreenMenuLangAndTime()
    : ScreenMenuLangAndTime__(_(label)) {
    EnableLongHoldScreenAction();
}

void ScreenMenuLangAndTime::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    if (event == GUI_event_t::LOOP) {
#if PRINTER_IS_PRUSA_MINI()
        // Label MI_TIME_NOW doesn't have its own windowEvent, so update the time here.
        if (time_tools::update_time()) {
            Item<MI_TIME_NOW>().Invalidate();
        }
#endif /* PRINTER_IS_PRUSA_MINI() */
    }
}
