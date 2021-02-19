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
    void wait_until_closed(ClientFSM dialog, uint8_t data);
    void command(fsm::variant_t variant);

public:
    //accessor for static methods
    static DialogHandler &Access();
    //static methods to be registered as callbacks
    static void Command(uint32_t data);                  //marlin client is in C, so cannot pass fsm::variant_t
    static void Loop();                                  //synchronization loop, call it outside event
    static void PreOpen(ClientFSM dialog, uint8_t data); //can be enforced (pre openned), unlike change/close

    // opens dialog, waits until closed, auto loops
    // avoid calling multiple times inside one method (event), could cause dialog memory corruption
    // TODO rewrite!!!
    static void WaitUntilClosed(ClientFSM dialog, uint8_t data);
};
