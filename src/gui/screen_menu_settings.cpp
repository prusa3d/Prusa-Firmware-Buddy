/**
 * @file screen_menu_settings.cpp
 */

#include "screen_menu_settings.hpp"
#include "ScreenHandler.hpp"
#include "netdev.h"
#include "wui.h"

#ifdef BUDDY_ENABLE_CONNECT
    #include <connect/marlin_printer.hpp>
#endif

#include "screen_menus.hpp"

MI_LOAD_SETTINGS::MI_LOAD_SETTINGS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_LOAD_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    // FIXME: Some error handling/reporting
    // TODO: Loading other things than just network
    if (netdev_load_ini_to_eeprom()) {
        notify_reconfigure();
    }

    // FIXME: Error handling
#ifdef BUDDY_ENABLE_CONNECT
    connect_client::MarlinPrinter::load_cfg_from_ini();
#endif
}

ScreenMenuSettings::ScreenMenuSettings()
    : ScreenMenuSettings__(_(label))
    , old_action(gui::knob::GetLongPressScreenAction()) { // backup hold action

    gui::knob::RegisterLongPressScreenAction([]() { Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuExperimentalSettings>); }); // new hold action
    EnableLongHoldScreenAction();
}

ScreenMenuSettings::~ScreenMenuSettings() {
    gui::knob::RegisterLongPressScreenAction(old_action); // restore hold action
}
