#pragma once

#include <stdint.h>
#include "fsm_types.hpp"
#include "DialogFactory.hpp"

class DialogHandler {
    static_unique_ptr<IDialogMarlin> ptr;
    DialogFactory::Ctors dialog_ctors;
    ClientFSM waiting_closed = ClientFSM::_none;
    fsm::Queue command_queue;

    DialogHandler(DialogFactory::Ctors ctors)
        : dialog_ctors(ctors) {}
    DialogHandler(DialogHandler &) = delete;

    void close(fsm::destroy_t o);
    void change(fsm::change_t o);
    void open(fsm::create_t o); //can be enforced (pre openned), unlike change/close
    void command(fsm::variant_t variant);

public:
    //accessor for static methods
    static DialogHandler &Access();
    static void Command(uint32_t u32, uint16_t u16);     //static method to be registered as callback, marlin client is in C, so cannot pass fsm::variant_t
    static void PreOpen(ClientFSM dialog, uint8_t data); //can be enforced (pre openned), unlike change/close

    void Loop();                                          //synchronization loop, call it outside event
    void WaitUntilClosed(ClientFSM dialog, uint8_t data); // opens dialog, waits until closed, auto loops
};
