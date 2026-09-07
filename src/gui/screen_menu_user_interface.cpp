/// @file
#include "screen_menu_user_interface.hpp"

#include <img_resources.hpp>

ScreenMenuUserInterface::ScreenMenuUserInterface()
    : ScreenMenuUserInterface__ {
        _("USER INTERFACE"),
        &img::settings_16x16,
    } {
    EnableLongHoldScreenAction();
}
