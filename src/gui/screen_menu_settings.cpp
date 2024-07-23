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
#if PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_MINI()
    Screens::Access()->Open(ScreenFactory::Screen<ScreenHelpFWUpdate>);
#endif
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

void ScreenMenuSettings::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
#if HAS_MMU2()
        // If filament sensor gets disabled, set the MMU enable to false as well
        if (!Item<MI_FILAMENT_SENSOR>().GetIndex()) {
            Item<MI_MMU_ENABLE>().SetIndex(0);
        }
#endif

    } else {
        ScreenMenu::windowEvent(sender, event, param);
    }
}
