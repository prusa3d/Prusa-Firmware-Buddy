// screen_menu_filament.cpp

#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "filament.hpp"
#include "filament_sensor_api.hpp"
#include "i18n.h"
#include "MItem_filament.hpp"

enum {
    F_EEPROM = 0x01, // filament is known
    F_SENSED = 0x02  // filament is not in sensor
};

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE>;

class ScreenMenuFilament : public Screen {
public:
    constexpr static const char *label = N_("FILAMENT");
    ScreenMenuFilament()
        : Screen(_(label)) {
        deactivate_item();
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void deactivate_item();
};

void ScreenMenuFilament::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    // This check is periodically executed even when it's hidden under filament dialogs.
    // It is a valid behaviour, but be aware, it can promote GUI bugs.
    // If it manifests invalidation bugs like blinking - fix GUI or don't execute when dialog is open
    deactivate_item();

    if (event == GUI_event_t::CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            item->Do();               //do action (load filament ...)
            header.SetText(_(label)); //restore label
        }
        return;
    }

    SuperWindowEvent(sender, event, param);
}

/*****************************************************************************/
//non-static method definition

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

    uint8_t filament = 0;
    filament |= Filaments::CurrentIndex() != filament_t::NONE ? F_EEPROM : 0;
    filament |= FSensors_instance().HasFilament() ? F_SENSED : 0;
    switch (filament) {
    case 0:        //filament not loaded
    case F_SENSED: //user pushed filament into sensor, but it is not loaded
        DisableItem<MI_CHANGE>();
        DisableItem<MI_PURGE>();
        break;
    case F_EEPROM: //filament loaded but just runout
        EnableItem<MI_CHANGE>();
        DisableItem<MI_PURGE>();
        break;
    case F_SENSED | F_EEPROM: //filament loaded
    default:
        EnableItem<MI_CHANGE>();
        EnableItem<MI_PURGE>();
        break;
    }
}

ScreenFactory::UniquePtr GetScreenMenuFilament() {
    return ScreenFactory::Screen<ScreenMenuFilament>();
}
