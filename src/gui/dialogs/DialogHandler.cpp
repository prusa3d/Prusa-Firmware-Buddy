#include "DialogHandler.h"
#include "gui.h"
#include "DialogLoadUnload.hpp"
#include "DialogFactory.hpp"

//screens do not have headers, have to use extern
extern screen_t *pscreen_printing_serial;

//*****************************************************************************
//DialogHandler declaration
class DialogHandler {
    static_unique_ptr<IDialogStateful> ptr;
    DialogFactory::Ctors dialog_ctors;

public:
    DialogHandler(DialogFactory::Ctors ctors)
        : dialog_ctors(ctors) {}

    void open(dialog_t dialog, uint8_t data);
    void close(dialog_t dialog);
    void change(dialog_t dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);
};

//*****************************************************************************
//Meyers singleton
DialogHandler &dlg_hndlr() {
    static DialogHandler ret(DialogFactory::GetAll());
    return ret;
}

//*****************************************************************************
//method definitions
void DialogHandler::open(dialog_t dialog, uint8_t data) {
    if (ptr)
        return; //an dialog is already openned

    if (gui_get_nesting() > 1) //another test if dialog is openned todo remove after gui refactoring
        return;

    //todo pscreen_printing_serial is no dialog but screen ... change to dialog?
    if ((screen_get_curr() == pscreen_printing_serial))
        return;

    //todo pscreen_printing_serial is no dialog but screen ... change to dialog?
    // only ptr = dialog_creators[dialog](data); should remain
    if (dialog == DLG_serial_printing) {
        screen_unloop(m876_blacklist, m876_blacklist_sz);

        if (screen_get_curr() != pscreen_printing_serial)
            screen_open(pscreen_printing_serial->id);
    } else {
        ptr = dialog_ctors[dialog](data);
    }
    /*
    if (dialog == DLG_load_unload)
        DialogFactory::load_unload(data);*/
}

void DialogHandler::close(dialog_t dialog) {
    if (gui_get_nesting() > 1) //test if dialog is openned todo remove after gui refactoring
        return;

    //hack pscreen_printing_serial is no dialog but screen ... todo change to dialog?
    if (dialog == DLG_serial_printing) {
        if (screen_get_curr() == pscreen_printing_serial)
            screen_close();
    }

    ptr = nullptr; //destroy current dialog
}

void DialogHandler::change(dialog_t dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    if (ptr)
        ptr->Change(phase, progress_tot, progress);
}

//functions for C API
extern "C" {
void dialog_open_cb(dialog_t dialog, uint8_t data) {
    dlg_hndlr().open(dialog, data);
}

void dialog_close_cb(dialog_t dialog) {
    dlg_hndlr().close(dialog);
}

void dialog_change_cb(dialog_t dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    dlg_hndlr().change(dialog, phase, progress_tot, progress);
}
}
