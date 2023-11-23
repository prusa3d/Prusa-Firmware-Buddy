#pragma once

#include <stdint.h>
#include "fsm_types.hpp"
#include "DialogFactory.hpp"

class DialogHandler {
    uint32_t opened_times[size_t(ClientFSM::_count)] = {}; // to be able to make blocking dialog
    uint32_t closed_times[size_t(ClientFSM::_count)] = {}; // to be able to make blocking dialog
    static_unique_ptr<IDialogMarlin> ptr;
    DialogFactory::Ctors dialog_ctors;
    ClientFSM waiting_closed = ClientFSM::_none;
    fsm::SmartQueue command_queue;

    DialogHandler(DialogFactory::Ctors ctors)
        : dialog_ctors(ctors) {}
    DialogHandler(DialogHandler &) = delete;

    void close(ClientFSM fsm_type);
    void change(ClientFSM fsm_type, fsm::BaseData data);
    void open(ClientFSM fsm_type, fsm::BaseData data); // can be enforced (pre opened), unlike change/close
    void command(fsm::DequeStates change);

public:
    // accessor for static methods
    static DialogHandler &Access();
    static void Command(std::pair<uint32_t, uint16_t> serialized);
    static void command_c_compatible(uint32_t u32, uint16_t u16) { Command({ u32, u16 }); } // static method to be registered as callback, marlin client is in C, so cannot pass std::pair
    static void PreOpen(ClientFSM dialog, fsm::BaseData data); // can be enforced (pre opened), unlike change/close

    void Loop(); // synchronization loop, call it outside event
    void WaitUntilClosed(ClientFSM dialog, fsm::BaseData data); // opens dialog, waits until closed, auto loops
    bool IsOpen() const; // returns true if any dialog is active (we dont want popups)

    uint32_t OpenedTimes(ClientFSM fsm) const { return opened_times[size_t(fsm)]; }
    uint32_t ClosedTimes(ClientFSM fsm) const { return closed_times[size_t(fsm)]; }
    bool IsOpen(ClientFSM fsm) const;
    bool IsAnyOpen() const;
};
