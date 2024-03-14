/**
 * @file menu_opener.cpp
 */

#include "menu_opener.hpp"
#include "ScreenHandler.hpp"

void open_screen(const ScreenCreator open_fn) {
    Screens::Access()->Open(open_fn);
}
