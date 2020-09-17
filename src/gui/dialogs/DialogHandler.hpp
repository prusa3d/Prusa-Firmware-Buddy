#pragma once

#include <stdint.h>
#include "client_fsm_types.h"
#include "DialogFactory.hpp"

class DialogHandler {
    static_unique_ptr<IDialogMarlin> ptr;
    DialogFactory::Ctors dialog_ctors;
    ClientFSM waiting_closed = ClientFSM::_none;

    DialogHandler(DialogFactory::Ctors ctors)
        : dialog_ctors(ctors) {}
    DialogHandler(DialogHandler &) = delete;

    void open(ClientFSM dialog, uint8_t data);
    void close(ClientFSM dialog);
    void change(ClientFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);

    void wait_until_closed(ClientFSM dialog, uint8_t data);

public:
    //accessor for static methods
    static DialogHandler &Access();
    //static methods to be registerd as callbacks
    static void Open(ClientFSM dialog, uint8_t data);
    static void Close(ClientFSM dialog);
    static void Change(ClientFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);
    static void WaitUntilClosed(ClientFSM dialog, uint8_t data); //opens dialog, waits until closed, auto loops
};
