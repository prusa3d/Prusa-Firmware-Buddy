#include "DialogHandler.hpp"
#include "gui.h"
#include "DialogLoadUnload.hpp"
#include "DialogFactory.hpp"
#include "screens.h"
#include "screen_close_multiple.h"

//*****************************************************************************
//method definitions
void DialogHandler::open(ClientFSM dialog, uint8_t data) {
    if (ptr)
        return; //an dialog is already openned

    if (gui_get_nesting() > 1) //another test if dialog is openned todo remove after gui refactoring
        return;

    //todo get_scr_printing_serial() is no dialog but screen ... change to dialog?
    if ((screen_get_curr() == get_scr_printing_serial()))
        return;

    //todo get_scr_printing_serial() is no dialog but screen ... change to dialog?
    // only ptr = dialog_creators[dialog](data); should remain
    if (dialog == ClientFSM::Serial_printing) {
        screen_close_multiple(scrn_close_on_M876);

        if (screen_get_curr() != get_scr_printing_serial())
            screen_open(get_scr_printing_serial()->id);
    } else {
        ptr = dialog_ctors[size_t(dialog)](data);
    }
}

void DialogHandler::close(ClientFSM dialog) {
    if (gui_get_nesting() > 1) //test if dialog is openned todo remove after gui refactoring
        return;

    //hack get_scr_printing_serial() is no dialog but screen ... todo change to dialog?
    if (dialog == ClientFSM::Serial_printing) {
        if (screen_get_curr() == get_scr_menu_tune())
            screen_close();

        if (screen_get_curr() == get_scr_printing_serial())
            screen_close();
    }

    ptr = nullptr; //destroy current dialog
}

void DialogHandler::change(ClientFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    if (ptr)
        ptr->Change(phase, progress_tot, progress);
}

//*****************************************************************************
//Meyers singleton
DialogHandler &DialogHandler::Access() {
    static DialogHandler ret(DialogFactory::GetAll());
    return ret;
}

void DialogHandler::Open(ClientFSM dialog, uint8_t data) {
    Access().open(dialog, data);
}
void DialogHandler::Close(ClientFSM dialog) {
    Access().close(dialog);
}
void DialogHandler::Change(ClientFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    Access().change(dialog, phase, progress_tot, progress);
}
