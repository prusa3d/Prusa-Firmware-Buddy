#include "DialogHandler.hpp"
#include "gui.h"
#include "DialogLoadUnload.hpp"
#include "DialogFactory.hpp"

//screens do not have headers, have to use extern
extern "C" {
extern screen_t *pscreen_menu_tune;
extern screen_t *pscreen_printing_serial;
}

//*****************************************************************************
//method definitions
void DialogHandler::open(ClinetFSM dialog, uint8_t data) {
    if (ptr)
        return; //an dialog is already openned

    if (gui_get_nesting() > 1) //another test if dialog is openned todo remove after gui refactoring
        return;

    //todo pscreen_printing_serial is no dialog but screen ... change to dialog?
    if ((screen_get_curr() == pscreen_printing_serial))
        return;

    //todo pscreen_printing_serial is no dialog but screen ... change to dialog?
    // only ptr = dialog_creators[dialog](data); should remain
    if (dialog == ClinetFSM::Serial_printing) {
        screen_unloop(m876_blacklist, m876_blacklist_sz);

        if (screen_get_curr() != pscreen_printing_serial)
            screen_open(pscreen_printing_serial->id);
    } else {
        ptr = dialog_ctors[size_t(dialog)](data);
    }
}

void DialogHandler::close(ClinetFSM dialog) {
    if (gui_get_nesting() > 1) //test if dialog is openned todo remove after gui refactoring
        return;

    //hack pscreen_printing_serial is no dialog but screen ... todo change to dialog?
    if (dialog == ClinetFSM::Serial_printing) {
        if (screen_get_curr() == pscreen_menu_tune)
            screen_close();

        if (screen_get_curr() == pscreen_printing_serial)
            screen_close();
    }

    ptr = nullptr; //destroy current dialog
}

void DialogHandler::change(ClinetFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    if (ptr)
        ptr->Change(phase, progress_tot, progress);
}

//*****************************************************************************
//Meyers singleton
DialogHandler &DialogHandler::Access() {
    static DialogHandler ret(DialogFactory::GetAll());
    return ret;
}

void DialogHandler::Open(ClinetFSM dialog, uint8_t data) {
    Access().open(dialog, data);
}
void DialogHandler::Close(ClinetFSM dialog) {
    Access().close(dialog);
}
void DialogHandler::Change(ClinetFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    Access().change(dialog, phase, progress_tot, progress);
}
