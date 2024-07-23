/**
 * @file screen_menu_filament.cpp
 */

#include "screen_menu_filament.hpp"
#include "filament.hpp"
#include "filament_sensors_handler.hpp"

#include "img_resources.hpp"

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <window_tool_action_box.hpp>
#endif

enum {
    F_EEPROM = 0x01, // filament is known
    F_SENSED = 0x02 // filament is not in sensor
};

ScreenMenuFilament::ScreenMenuFilament()
    : ScreenMenuFilament__(_(label)) {
#if (!PRINTER_IS_PRUSA_MINI())
    header.SetIcon(&img::spool_white_16x16);
#endif // PRINTER_IS_PRUSA_MINI()
    deactivate_item();
}

void ScreenMenuFilament::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    // This check is periodically executed even when it's hidden under filament dialogs.
    // It is a valid behaviour, but be aware, it can promote GUI bugs.
    // If it manifests invalidation bugs like blinking - fix GUI or don't execute when dialog is open
    deactivate_item();

    if (event == GUI_event_t::CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            auto menu_index = menu.menu.focused_item_index();

            item->Do(); // do action (load filament ...)

            menu.menu.move_focus_to_index(menu_index.value_or(0)); // restore menu index
            header.SetText(_(label)); // restore label
        }
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

/*****************************************************************************/
// non-static method definition

/*
 * +---------+--------++------------+--------+--------+-------+--------------------------------------------------------+
 * | FSENSOR | EEPROM || load       | unload | change | purge | comment                                                |
 * +---------+--------++------------+--------+--------+-------+--------------------------------------------------------+
 * |       0 |      0 ||  YES       |    YES |     NO |    NO | filament not loaded                                    |
 * |       0 |      1 ||  YES (ASK) |    YES |    YES |    NO | filament loaded but just runout                        |
 * |       1 |      0 ||  YES       |    YES |     NO |    NO | user pushed filament into sensor, but it is not loaded |
 * |       1 |      1 ||  YES (ASK) |    YES |    YES |   YES | filament loaded                                        |
 * +---------+--------++------------+--------+--------+-------+--------------------------------------------------------+
 */
void ScreenMenuFilament::deactivate_item() {
    Item<MI_CHANGE>().UpdateEnableState();
    Item<MI_PURGE>().UpdateEnableState();
}
