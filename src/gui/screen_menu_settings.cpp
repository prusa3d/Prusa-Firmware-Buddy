/**
 * @file screen_menu_settings.cpp
 */

#include "screen_menu_settings.hpp"
#include "screen_menu_experimental_settings.hpp"
#include "screen_help_fw_update.hpp"
#include "ScreenHandler.hpp"
#include "netdev.h"
#include "wui.h"

#include "knob_event.hpp"
#include "img_resources.hpp"

/*****************************************************************************/
// MI_HELP_FW_UPDATE
MI_HELP_FW_UPDATE::MI_HELP_FW_UPDATE()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_HELP_FW_UPDATE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenHelpFWUpdate>);
}

ScreenMenuSettings::ScreenMenuSettings()
    : ScreenMenuSettings__(_(label))
    , old_action(gui::knob::GetLongPressScreenAction()) { // backup hold action

    EnableLongHoldScreenAction();

#if (!PRINTER_IS_PRUSA_MINI())
    header.SetIcon(&img::settings_16x16);
#endif // PRINTER_IS_PRUSA_MINI()

    gui::knob::RegisterLongPressScreenAction([]() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>); }); // new hold action
    EnableLongHoldScreenAction();
}

ScreenMenuSettings::~ScreenMenuSettings() {
    gui::knob::RegisterLongPressScreenAction(old_action); // restore hold action
}
