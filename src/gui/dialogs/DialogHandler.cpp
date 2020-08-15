#include "DialogHandler.hpp"
#include "gui.hpp"
#include "DialogLoadUnload.hpp"
#include "DialogFactory.hpp"
#include "IScreenPrinting.hpp"
#include "ScreenHandler.hpp"
#include "screen_printing_serial.hpp"
#include "screen_printing.hpp"

//*****************************************************************************
//method definitions
void DialogHandler::open(ClientFSM dialog, uint8_t data) {
    if (ptr)
        return; //an dialog is already openned

    if (gui_get_nesting() > 1) //another test if dialog is openned todo remove after gui refactoring
        return;

    //todo get_scr_printing_serial() is no dialog but screen ... change to dialog?
    // only ptr = dialog_creators[dialog](data); should remain
    if (dialog == ClientFSM::Serial_printing) {
        if (IScreenPrinting::CanOpen()) {
            Screens::Access()->CloseAll();
            Screens::Access()->Open(ScreenFactory::Screen<screen_printing_serial_data_t>);
        }
    } else if (dialog == ClientFSM::Printing) {
        if (IScreenPrinting::CanOpen()) {
            Screens::Access()->CloseAll();
            Screens::Access()->Open(ScreenFactory::Screen<screen_printing_data_t>);
        }
    } else {
        ptr = dialog_ctors[size_t(dialog)](data);
    }
}

void DialogHandler::close(ClientFSM dialog) {
    if (waiting_closed == dialog) {
        waiting_closed = ClientFSM::_none;
    } else {
        if (gui_get_nesting() > 1) //test if dialog is openned todo remove after gui refactoring
            return;

        //hack get_scr_printing_serial() is no dialog but screen ... todo change to dialog?
        if (dialog == ClientFSM::Serial_printing) {
            Screens::Access()->CloseAll();
        }
    }

    ptr = nullptr; //destroy current dialog
}

void DialogHandler::change(ClientFSM /*dialog*/, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    if (ptr)
        ptr->Change(phase, progress_tot, progress);
}

void DialogHandler::wait_until_closed(ClientFSM dialog, uint8_t data) {
    open(dialog, data);
    waiting_closed = dialog;
    while (waiting_closed == dialog)
        gui_loop();
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
void DialogHandler::WaitUntilClosed(ClientFSM dialog, uint8_t data) {
    Access().wait_until_closed(dialog, data);
}
