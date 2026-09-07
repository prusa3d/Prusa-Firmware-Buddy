/// @file
#include <screen_menu_system.hpp>

#include <img_resources.hpp>

ScreenMenuSystem::ScreenMenuSystem()
    : ScreenMenuSystemBase {
        _("SYSTEM"),
        &img::settings_16x16,
    } {
    EnableLongHoldScreenAction();
}
