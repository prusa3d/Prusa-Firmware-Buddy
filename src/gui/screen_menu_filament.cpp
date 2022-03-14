// screen_menu_filament.cpp

#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "filament.hpp"
#include "filament_sensor.hpp"
#include "marlin_client.h"
#include "window_dlg_load_unload.hpp"
#include "i18n.h"
#include "MItem_event_dispatcher.hpp"
#include "DialogHandler.hpp"
#include "sound.hpp"

enum {
    F_EEPROM = 0x01, // filament is known
    F_SENSED = 0x02  // filament is not in sensor
};

/*****************************************************************************/
//MI_LOAD
class MI_LOAD : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Load Filament");
    constexpr static const char *const warning_loaded = N_("Filament appears to be already loaded, are you sure you want to load it anyway?");

public:
    MI_LOAD()
        : MI_event_dispatcher(_(label)) {}
    virtual void Do() override {
        if ((Filaments::CurrentIndex() == filament_t::NONE) || (MsgBoxWarning(_(warning_loaded), Responses_YesNo, 1) == Response::Yes)) {
            PreheatStatus::Dialog(PreheatMode::Load, RetAndCool_t::Return);
        }
    }
};

/*****************************************************************************/
//MI_UNLOAD
class MI_UNLOAD : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Unload Filament");
    constexpr static const char *const header_label = N_("UNLOAD FILAMENT");

public:
    MI_UNLOAD()
        : MI_event_dispatcher(_(label)) {}
    virtual void Do() override {
        PreheatStatus::Dialog(PreheatMode::Unload, RetAndCool_t::Return);
        Sound_Stop(); // TODO what is Sound_Stop(); doing here?
    }
};

/*****************************************************************************/
//MI_CHANGE
class MI_CHANGE : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Change Filament");
    constexpr static const char *const header_label = N_("CHANGE FILAMENT");

public:
    MI_CHANGE()
        : MI_event_dispatcher(_(label)) {}
    virtual void Do() override {
        PreheatStatus::Dialog(PreheatMode::Change_phase1, RetAndCool_t::Return);
        Sound_Stop(); // TODO what is Sound_Stop(); doing here?
    }
};

/*****************************************************************************/
//MI_PURGE
class MI_PURGE : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Purge Filament");
    constexpr static const char *const header_label = N_("PURGE FILAMENT");

public:
    MI_PURGE()
        : MI_event_dispatcher(_(label)) {}
    virtual void Do() override {
        PreheatStatus::Dialog(PreheatMode::Purge, RetAndCool_t::Return);
    }
};

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE>;

class ScreenMenuFilament : public Screen {
public:
    constexpr static const char *label = N_("FILAMENT");
    ScreenMenuFilament()
        : Screen(_(label)) {
        Screen::ClrMenuTimeoutClose(); // don't close on menu timeout
        deactivate_item();
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void deactivate_item();
};

void ScreenMenuFilament::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
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
    filament |= FS_instance().Get() == fsensor_t::NoFilament ? 0 : F_SENSED;
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
