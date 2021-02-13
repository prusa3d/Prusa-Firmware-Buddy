// screen_menu_filament.cpp

#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "filament.hpp"
#include "filament_sensor.hpp"
#include "marlin_client.h"
#include "window_dlg_load_unload.hpp"
#include "dbg.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "DialogHandler.hpp"
#include "sound.hpp"

enum {
    F_EEPROM = 0x01, // filament is known
    F_SENSED = 0x02  // filament is not in sensor
};

/*****************************************************************************/
//parent
class MI_event_dispatcher : public WI_LABEL_t {
protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        //no way to change header on this level, have to dispatch event
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CLICK, (void *)this); //WI_LABEL is not a window, cannot set sender param
    }

public:
    explicit MI_event_dispatcher(string_view_utf8 label)
        : WI_LABEL_t(label, 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void Do() = 0;
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

using Screen = ScreenMenu<EHeader::Off, EFooter::On, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE>;

class ScreenMenuFilament : public Screen {
public:
    constexpr static const char *label = N_("FILAMENT");
    ScreenMenuFilament()
        : Screen(_(label)) {
        if (marlin_vars()->fs_autoload_enabled == 1 && (FS_instance().Get() == fsensor_t::HasFilament || FS_instance().Get() == fsensor_t::NoFilament)) {
            Item<MI_LOAD>().Hide();
            Item<MI_CHANGE>().Hide();
        }
        Screen::ClrMenuTimeoutClose(); // don't close on menu timeout
        deactivate_item();
    }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void deactivate_item();

    template <class T>
    void dis() {
        if (Item<T>().IsEnabled()) {
            Item<T>().Disable();
            Invalidate();
        }
    }
    template <class T>
    void ena() {
        if (!Item<T>().IsEnabled()) {
            Item<T>().Enable();
            Invalidate();
        }
    }
};

void ScreenMenuFilament::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    deactivate_item();
    if (event == GUI_event_t::CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            item->Do();               //do action (load filament ...)
            header.SetText(_(label)); //restore label
        }
    } else {
        SuperWindowEvent(sender, event, param);
    }
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
        dis<MI_CHANGE>();
        dis<MI_PURGE>();
        break;
    case F_EEPROM: //filament loaded but just runout
        ena<MI_CHANGE>();
        dis<MI_PURGE>();
        break;
    case F_SENSED | F_EEPROM: //filament loaded
    default:
        ena<MI_CHANGE>();
        ena<MI_PURGE>();
        break;
    }
}

ScreenFactory::UniquePtr GetScreenMenuFilament() {
    return ScreenFactory::Screen<ScreenMenuFilament>();
}
