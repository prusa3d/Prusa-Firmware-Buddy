#include "DialogHandler.h"
#include "gui.h"
#include "DialogLoadUnload.hpp"
#include "DialogFactory.hpp"

//screens do not have headers, have to use extern
extern screen_t *pscreen_printing_serial;

//*****************************************************************************
//DialogHandler declaration
class DialogHandler {
    dialog_t opened;
    static_unique_ptr<IDialogStateful> ptr;

public:
    DialogHandler()
        : opened(DLG_count)
    //: ptr(make_static_unique_ptr<DialogNONE>(&all_dialogs))
    {}

    void open(dialog_t dialog, uint8_t data);
    void close(dialog_t dialog);
    void change(dialog_t dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);
};

//*****************************************************************************
//Meyers singleton
DialogHandler &dlg_hndlr() {
    static DialogHandler ret;
    return ret;
}

//*****************************************************************************
//method definitions
void DialogHandler::open(dialog_t dialog, uint8_t data) {
    if (ptr)
        return; //an dialog is already openned

    if (opened != DLG_count)
        return;

    if (gui_get_nesting() > 1) //another test if dialog is openned todo remove after gui refactoring
        return;

    opened = dialog;

    //todo pscreen_printing_serial is no dialog but screen ... change to dialog?
    if (dialog == DLG_serial_printing) {
        screen_unloop(m876_blacklist, m876_blacklist_sz);

        if (screen_get_curr() != pscreen_printing_serial)
            screen_open(pscreen_printing_serial->id);
    }

    if (dialog == DLG_load_unload)
        DialogFactory::load_unload(data);
}

void DialogHandler::close(dialog_t dialog) {
    if (gui_get_nesting() > 1) //test if dialog is openned todo remove after gui refactoring
        return;

    //todo pscreen_printing_serial is no dialog but screen ... change to dialog?
    if (dialog == DLG_serial_printing) {
        if (screen_get_curr() == pscreen_printing_serial)
            screen_close();
    }

    ptr = nullptr; //destroy current dialog
    opened = DLG_count;
}

void DialogHandler::change(dialog_t dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    if (opened != dialog)
        return;
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
