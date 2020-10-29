// screen_menu_filament.cpp

#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "filament.h"
#include "filament_sensor.hpp"
#include "marlin_client.h"
#include "window_dlg_load_unload.h"
#include "dbg.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"

/// Sets temperature of nozzle not to ooze before print (MBL)
void setPreheatTemp() {
    /// read from Marlin, not from EEPROM since it's not in sync
    marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
    marlin_gcode_printf("M104 S%d D%d", (int)PREHEAT_TEMP, (int)vars->target_nozzle);
}
void clrPreheatTemp() {
    marlin_gcode("M104 S0");
}

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
        : WI_LABEL_t(label, 0, true, false) {}

    virtual string_view_utf8 GetHeaderAlterLabel() = 0;
    virtual void Do() = 0;
};

/*****************************************************************************/
//MI_LOAD
class MI_LOAD : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Load Filament");
    constexpr static const char *const header_label = N_("LOAD FILAMENT");

public:
    MI_LOAD()
        : MI_event_dispatcher(_(label)) {}
    virtual string_view_utf8 GetHeaderAlterLabel() override {
        return _(header_label);
    }
    virtual void Do() override {
        gui_dlg_load() == DLG_OK ? setPreheatTemp() : clrPreheatTemp();
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
    virtual string_view_utf8 GetHeaderAlterLabel() override {
        return _(header_label);
    }
    virtual void Do() override {
        gui_dlg_unload();
        Sound_Stop();
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
    virtual string_view_utf8 GetHeaderAlterLabel() override {
        return _(header_label);
    }
    virtual void Do() override {
        if (gui_dlg_unload() == DLG_OK) {
            Sound_Stop();
            gui_dlg_load() == DLG_OK ? setPreheatTemp() : clrPreheatTemp();
        }
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
    virtual string_view_utf8 GetHeaderAlterLabel() override {
        return _(header_label);
    }
    virtual void Do() override {
        gui_dlg_purge() == DLG_OK ? setPreheatTemp() : clrPreheatTemp();
    }
};

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE>;

class ScreenMenuFilament : public Screen {
    enum {
        F_EEPROM = 0x01, // filament is known
        F_SENSED = 0x02  // filament is not in sensor
    };

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

ScreenFactory::UniquePtr GetScreenMenuFilament() {
    return ScreenFactory::Screen<ScreenMenuFilament>();
}

void ScreenMenuFilament::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    deactivate_item();
    if (event == GUI_event_t::CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            header.SetText(item->GetHeaderAlterLabel()); //set new label
            item->Do();                                  //do action (load filament ...)
            header.SetText(_(label));                    //restore label
        }
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//non-static method definition

/*
 * +---------+--------++------+--------+--------+-------+--------------------------------------------------------+
 * | FSENSOR | EEPROM || load | unload | change | purge | comment                                                |
 * +---------+--------++------+--------+--------+-------+--------------------------------------------------------+
 * |       0 |      0 ||  YES |    YES |     NO |    NO | filament not loaded                                    |
 * |       0 |      1 ||   NO |    YES |    YES |    NO | filament loaded but just runout                        |
 * |       1 |      0 ||  YES |    YES |     NO |    NO | user pushed filament into sensor, but it is not loaded |
 * |       1 |      1 ||   NO |    YES |    YES |   YES | filament loaded                                        |
 * +---------+--------++------+--------+--------+-------+--------------------------------------------------------+
 */
void ScreenMenuFilament::deactivate_item() {

    uint8_t filament = 0;
    filament |= get_filament() != FILAMENT_NONE ? F_EEPROM : 0;
    filament |= fs_get_state() == fsensor_t::NoFilament ? 0 : F_SENSED;
    switch (filament) {
    case 0: //filament not loaded
        ena<MI_LOAD>();
        dis<MI_CHANGE>();
        dis<MI_PURGE>();
        break;
    case F_EEPROM: //filament loaded but just runout
        dis<MI_LOAD>();
        ena<MI_CHANGE>();
        dis<MI_PURGE>();
        break;
    case F_SENSED: //user pushed filament into sensor, but it is not loaded
        ena<MI_LOAD>();
        dis<MI_CHANGE>();
        dis<MI_PURGE>();
        break;
    case F_SENSED | F_EEPROM: //filament loaded
    default:
        dis<MI_LOAD>();
        ena<MI_CHANGE>();
        ena<MI_PURGE>();
        break;
    }
}
