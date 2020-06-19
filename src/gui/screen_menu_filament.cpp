// screen_menu_filament.c

#include "gui.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "filament.h"
#include "filament_sensor.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_dlg_load_unload.h"
#include "screens.h"
#include "dbg.h"
#include "DialogHandler.hpp"
#include "../lang/i18n.h"

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
        screen_dispatch_event(nullptr, WINDOW_EVENT_CLICK, (void *)this);
    }

public:
    explicit MI_event_dispatcher(const char *label)
        : WI_LABEL_t(label, 0, true, false) {}

    virtual const char *GetHeaderAlterLabel() = 0;
    virtual void Do() = 0;
};

/*****************************************************************************/
//MI_LOAD
class MI_LOAD : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Load Filament");
    constexpr static const char *const header_label = N_("LOAD FILAMENT");

public:
    MI_LOAD()
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLabel() override {
        return header_label;
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
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLabel() override {
        return header_label;
    }
    virtual void Do() override {
        gui_dlg_unload();
    }
};

/*****************************************************************************/
//MI_CHANGE
class MI_CHANGE : public MI_event_dispatcher {
    constexpr static const char *const label = N_("Change Filament");
    constexpr static const char *const header_label = N_("CHANGE FILAMENT");

public:
    MI_CHANGE()
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLabel() override {
        return header_label;
    }
    virtual void Do() override {
        if (gui_dlg_unload() == DLG_OK) {
            //opens unload dialog if it is not already openned
            DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Unload));

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
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLabel() override {
        return header_label;
    }
    virtual void Do() override {
        gui_dlg_purge() == DLG_OK ? setPreheatTemp() : clrPreheatTemp();
    }
};

using parent = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE>;

class ScreenMenuFilament : public parent {
    enum {
        F_EEPROM = 0x01, // filament is known
        F_SENSED = 0x02  // filament is not in sensor
    };

public:
    constexpr static const char *label = N_("FILAMENT");
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);

private:
    void deactivate_item();

    template <class T>
    void dis() {
        Item<T>().Disable();
        win.f_invalid = 1;
    }
    template <class T>
    void ena() {
        Item<T>().Enable();
        win.f_invalid = 1;
    }
};

/*****************************************************************************/
//static method definition
void ScreenMenuFilament::Init(screen_t *screen) {
    Create(screen, label);
    reinterpret_cast<ScreenMenuFilament *>(screen->pdata)->deactivate_item();
}

int ScreenMenuFilament::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuFilament *const ths = reinterpret_cast<ScreenMenuFilament *>(screen->pdata);
    ths->deactivate_item();
    if (event == WINDOW_EVENT_CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            p_window_header_set_text(&ths->header, item->GetHeaderAlterLabel()); //set new label
            item->Do();                                                          //do action (load filament ...)
            p_window_header_set_text(&ths->header, label);                       //restore label
        }
    } else {
        return ths->Event(window, event, param);
    }
    return 0;
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
    filament |= fs_get_state() == FS_NO_FILAMENT ? 0 : F_SENSED;
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
    gui_invalidate();
}

screen_t screen_menu_filament = {
    0,
    0,
    ScreenMenuFilament::Init,
    ScreenMenuFilament::CDone,
    ScreenMenuFilament::CDraw,
    ScreenMenuFilament::CEvent,
    sizeof(ScreenMenuFilament), //data_size
    nullptr,                    //pdata
};

screen_t *const get_scr_menu_filament() { return &screen_menu_filament; }
