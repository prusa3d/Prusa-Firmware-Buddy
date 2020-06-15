// wizard_load_unload.c

#include "wizard_load_unload.h"
#include "wizard_ui.h"
#include "window_dlg_load_unload.h"
#include "window_dlg_preheat.h"
#include "filament.h"
#include "filament_sensor.h"
#include "../lang/i18n.h"

#define FKNOWN      0x01 //filament is known
#define F_NOTSENSED 0x02 //filament is not in sensor

LD_UNLD_STATE_t _decide_continue_load_unload() {
    uint8_t filament = 0;
    filament |= get_filament() != FILAMENT_NONE ? FKNOWN : 0;
    filament |= fs_get_state() == FS_NO_FILAMENT ? F_NOTSENSED : 0;
    uint16_t def_bt = filament == (FKNOWN | F_NOTSENSED) ? MSGBOX_DEF_BUTTON2 : MSGBOX_DEF_BUTTON1;
    switch (filament) {
    case FKNOWN: { //known and not "unsensed" - do not allow load
        const char *btns[2] = { "NEXT", "UNLOAD" };
        switch (wizard_msgbox_btns(
            "To calibrate with  \n"
            "currently loaded   \n"
            "filament,          \n"
            "press NEXT.        \n"
            "To change filament,\n"
            "press UNLOAD.",
            MSGBOX_BTN_CUSTOM2, 0, btns)) {
        case MSGBOX_RES_CUSTOM0:
            return LD_UNLD_DONE;
        case MSGBOX_RES_CUSTOM1:
            return LD_UNLD_DIALOG_UNLOAD;
        default:
            //should not happen
            return LD_UNLD_DIALOG_PREHEAT;
        }
    }
    case FKNOWN | F_NOTSENSED: //allow load, prepick UNLOAD, force ask preheat
    case F_NOTSENSED:          //allow load, prepick LOAD, force ask preheat
    case 0:                    //filament is not known but is sensed == most likely same as F_NOTSENSED, but user inserted filament into sensor
    default: {
        //cannot use CONTINUE button, string is too long
        const char *btns[3] = { "NEXT", "LOAD", "UNLOAD" };
        switch (wizard_msgbox_btns(
            "To calibrate with  \n"
            "currently loaded   \n"
            "filament,          \n"
            "press NEXT.        \n"
            "To load filament,  \n"
            "press LOAD.        \n"
            "To change filament,\n"
            "press UNLOAD.",
            MSGBOX_BTN_CUSTOM3 | def_bt, 0, btns)) {
        case MSGBOX_RES_CUSTOM0:
            return LD_UNLD_DIALOG_PREHEAT;
        case MSGBOX_RES_CUSTOM1:
            return LD_UNLD_DIALOG_LOAD;
        case MSGBOX_RES_CUSTOM2:
            return LD_UNLD_DIALOG_UNLOAD;
        default:
            //should not happen
            return LD_UNLD_DIALOG_PREHEAT;
        }
    }
    }
}

LD_UNLD_STATE_t wizard_load_unload(LD_UNLD_STATE_t state) {
    switch (state) {
    case LD_UNLD_INIT:
        return LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD;

    case LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD:
        return _decide_continue_load_unload();
    case LD_UNLD_DIALOG_PREHEAT:
        gui_dlg_preheat_forced(_("Select filament type"));
        return LD_UNLD_DONE;

    case LD_UNLD_DIALOG_LOAD:
        switch (gui_dlg_load_forced()) {
        case DLG_OK:
            return LD_UNLD_DONE;
        case DLG_ABORTED:
            return LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD;
        }

    case LD_UNLD_DIALOG_UNLOAD:
        switch (gui_dlg_unload_forced()) {
        case DLG_OK:
            return LD_UNLD_DIALOG_LOAD;
        case DLG_ABORTED:
            return LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD;
        }

    case LD_UNLD_DONE:
    default:
        return state;
    }

    //dlg_load
}
