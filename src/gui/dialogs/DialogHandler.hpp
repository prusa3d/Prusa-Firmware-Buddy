#pragma once

#include <stdint.h>
#include "fsm_types.hpp"
#include "DialogFactory.hpp"

enum class redraw_cmd_t : int {
    none,
    skip,
    redraw,
};

class DialogHandler {
    uint32_t opened_times[size_t(ClientFSM::_count)] = {}; // to be able to make blocking dialog
    uint32_t closed_times[size_t(ClientFSM::_count)] = {}; // to be able to make blocking dialog
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

    redraw_cmd_t Loop();                                  //synchronization loop, call it outside event
    void WaitUntilClosed(ClientFSM dialog, uint8_t data); // opens dialog, waits until closed, auto loops
    bool IsOpen() const;                                  // returns true if any dialog is active (we dont want popups)

    uint32_t OpenedTimes(ClientFSM fsm) const { return opened_times[size_t(fsm)]; }
    uint32_t ClosedTimes(ClientFSM fsm) const { return closed_times[size_t(fsm)]; }
    uint32_t IsOpen(ClientFSM fsm) const { return OpenedTimes(fsm) != ClosedTimes(fsm); }
};
