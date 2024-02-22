#pragma once

#include <stdint.h>
#include "IDialogMarlin.hpp"
#include "fsm_types.hpp"
#include "screen_init_variant.hpp"
#include "static_alocation_ptr.hpp"

class DialogHandler {
    static_unique_ptr<IDialogMarlin> ptr;
    fsm::SmartQueue command_queue;

    DialogHandler() = default;
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

    void Loop(); // synchronization loop, call it outside event
    bool IsOpen() const; // returns true if any dialog is active (we dont want popups)

    bool IsOpen(ClientFSM fsm) const;
    bool IsAnyOpen() const;

private:
    /// When a dialog is opened, underlying screen state is saved. It then gets restored on dialog close
    screen_init_variant underlying_screen_state_;
};
