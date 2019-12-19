// wizard_load_unload.c

#include "wizard_load_unload.h"
#include "wizard_ui.h"
#include "window_dlg_load.h"
#include "window_dlg_unload.h"
#include "window_dlg_preheat.h"

LD_UNLD_STATE_t wizard_load_unload(LD_UNLD_STATE_t state) {
    switch (state) {
    case LD_UNLD_INIT:
        return LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD;

    case LD_UNLD_MSG_DECIDE_CONTINUE_LOAD_UNLOAD: {
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
            "press UNLOAD."

            ,
            MSGBOX_BTN_CUSTOM3, 0, btns)) {
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

    case LD_UNLD_DIALOG_PREHEAT:
        gui_dlg_preheat_forced("Select filament type");
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
