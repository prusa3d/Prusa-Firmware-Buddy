/**
 * @file screen_menu_experimental_settings.cpp
 * @author Radek Vana
 * @brief experimental settings
 * @date 2021-07-28
 */

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "screen_menus.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN>;

class ScreenMenuExperimentalSettings : public Screen {
public:
    constexpr static const char *label = N_("Experimental Settings");
    ScreenMenuExperimentalSettings()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuExperimentalSettings() {
    return ScreenFactory::Screen<ScreenMenuExperimentalSettings>();
}
