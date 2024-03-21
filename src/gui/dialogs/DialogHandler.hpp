#pragma once

#include <stdint.h>
#include "IDialogMarlin.hpp"
#include "fsm_states.hpp"
#include "static_alocation_ptr.hpp"

class DialogHandler {
    static_unique_ptr<IDialogMarlin> ptr;
    fsm::States fsm_states;
    std::pair<ClientFSM, fsm::BaseData> last_fsm_change;
    std::optional<std::pair<ClientFSM, fsm::BaseData>> dialog_cache;
    DialogHandler() = default;
    DialogHandler(DialogHandler &) = delete;

    void close(ClientFSM fsm_type);
    void change(ClientFSM fsm_type, fsm::BaseData data);
    void open(ClientFSM fsm_type, fsm::BaseData data); // can be enforced (pre opened), unlike change/close

public:
    // accessor for static methods
    static DialogHandler &Access();

    void Loop(); // synchronization loop, call it outside event
    bool IsOpen() const; // returns true if any dialog is active (we dont want popups)

    bool IsOpen(ClientFSM fsm) const;
    bool IsAnyOpen() const;
};
