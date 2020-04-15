#pragma once

#include <stdint.h>
#include "client_fsm_types.h"
#include "DialogFactory.hpp"

class DialogHandler {
    static_unique_ptr<IDialogStateful> ptr;
    DialogFactory::Ctors dialog_ctors;

    DialogHandler(DialogFactory::Ctors ctors)
        : dialog_ctors(ctors) {}
    DialogHandler(DialogHandler &) = delete;

    void open(ClinetFSM dialog, uint8_t data);
    void close(ClinetFSM dialog);
    void change(ClinetFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);

public:
    //accessor for static methods
    static DialogHandler &Access();
    //static methods to be registerd as callbacks
    static void Open(ClinetFSM dialog, uint8_t data);
    static void Close(ClinetFSM dialog);
    static void Change(ClinetFSM dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress);
};
