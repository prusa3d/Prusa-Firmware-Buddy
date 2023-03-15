/**
 * @file screen_menu_lang_and_time.cpp
 */

#include "screen_menu_lang_and_time.hpp"

ScreenMenuLangAndTime::ScreenMenuLangAndTime()
    : ScreenMenuLangAndTime__(_(label)) {
    EnableLongHoldScreenAction();
}
