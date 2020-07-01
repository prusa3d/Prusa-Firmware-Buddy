#if 0
    #include "ScreenHandler.hpp"
    #include "gui.hpp"
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
    if (waiting_closed == dialog) {
        waiting_closed = ClientFSM::_none;
    } else {
        if (gui_get_nesting() > 1) //test if dialog is openned todo remove after gui refactoring
            return;

        //hack get_scr_printing_serial() is no dialog but screen ... todo change to dialog?
        if (dialog == ClientFSM::Serial_printing) {
            if (screen_get_curr() == get_scr_menu_tune())
                screen_close();

            if (screen_get_curr() == get_scr_printing_serial())
                screen_close();
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
#endif

#include "ScreenHandler.hpp"
#include "bsod.h"

Screens *Screens::instance = nullptr;

Screens::Screens(ScreenFactory::Creator screen_creator)
    : stack({ { nullptr } })
    , current(screen_creator()) {
}

void Screens::Init(ScreenFactory::Creator screen_creator) {
    static Screens s(screen_creator);
    instance = &s;
}

Screens *Screens::Access() {
    if (!instance)
        bsod("Accessing uninitialized screen");
    return instance;
}

/*
void screen_dispatch_event(window_t *window, uint8_t event, void *param) {
    int ret = 0;
    if (screen_0 && screen_0->event) {
        ret = screen_0->event(screen_0, window, event, param);
        if (screen_0 == 0)
            ret = 1;
    }
    if ((ret == 0) && window && window->event)
        window->DispatchEvent(event, param);
}
*/
void Screens::DispatchEvent(uint8_t event, void *param) {
    Access()->current->Event(Access()->current.get(), event, param);
}

void Screens::Draw() {
    Access()->current->Draw();
}
