/**
 * @file screen_menu_settings.cpp
 */

#include "screen_menu_settings.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "ScreenHandler.hpp"
#include "netdev.h"
#include "wui.h"

#include "knob_event.hpp"
#include "SteelSheets.hpp"
#include "png_resources.hpp"

ScreenMenuSettings::ScreenMenuSettings()
    : ScreenMenuSettings__(_(label))
    , old_action(gui::knob::GetLongPressScreenAction()) { // backup hold action

    EnableLongHoldScreenAction();

#if (!PRINTER_IS_PRUSA_MINI)
    header.SetIcon(&png::settings_16x16);
#endif                                                                                                                                  // PRINTER_IS_PRUSA_MINI

    gui::knob::RegisterLongPressScreenAction([]() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>); }); // new hold action
    EnableLongHoldScreenAction();
}

ScreenMenuSettings::~ScreenMenuSettings() {
    gui::knob::RegisterLongPressScreenAction(old_action); // restore hold action
}
