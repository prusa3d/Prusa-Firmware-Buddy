/// @file
#include "screen_menu_settings.hpp"

#include "screen_menu_experimental_settings.hpp"
#include "ScreenHandler.hpp"
#include "knob_event.hpp"
#include "img_resources.hpp"

ScreenMenuSettings::ScreenMenuSettings()
    : ScreenMenuSettingsBase {
        _("SETTINGS"),
        &img::settings_16x16,
    }
    , old_action(gui::knob::GetLongPressScreenAction()) { // backup hold action
    gui::knob::RegisterLongPressScreenAction([]() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>); }); // new hold action
    EnableLongHoldScreenAction();
}

ScreenMenuSettings::~ScreenMenuSettings() {
    gui::knob::RegisterLongPressScreenAction(old_action); // restore hold action
}
