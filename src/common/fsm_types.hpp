/**
 * @file fsm_types.hpp
 * @author Radek Vana
 * @brief smart queue for fsm, stores and keeps only actual requests
 * @date 2021-02-18
 */

#pragma once

#include "client_fsm_types.h"
#include <stdint.h>
#include <array>

namespace fsm {

struct type_t {
    uint8_t command_and_type;
    constexpr type_t(ClientFSM_Command command, ClientFSM type)
        : command_and_type(uint8_t(type) | uint8_t(command)) {}
    constexpr ClientFSM_Command GetCommand() const { return ClientFSM_Command(command_and_type & uint8_t(ClientFSM_Command::_mask)); }
    constexpr ClientFSM GetType() const { return ClientFSM(command_and_type & (~uint8_t(ClientFSM_Command::_mask))); }
};

struct create_t {
    type_t type;
    uint8_t data;
    constexpr create_t(ClientFSM type_, uint8_t data)
        : type(type_t(ClientFSM_Command::create, type_))
        , data(data) {}
};

struct destroy_t {
    type_t type;
    constexpr destroy_t(ClientFSM type_)
        : type(type_t(ClientFSM_Command::destroy, type_)) {}
};

struct change_t {
    type_t type;
    uint8_t phase;
    uint8_t progress_tot;
    uint8_t progress;
    constexpr change_t(ClientFSM type_, uint8_t phase, uint8_t progress_tot, uint8_t progress)
        : type(type_t(ClientFSM_Command::change, type_))
        , phase(phase)
        , progress_tot(progress_tot)
        , progress(progress) {}
};

union variant_t {
    create_t create;
    destroy_t destroy;
    change_t change;
    uint32_t data;
    constexpr variant_t()
        : data(0) {} //ClientFSM_Command::none
    constexpr variant_t(create_t create)
        : create(create) {}
    constexpr variant_t(destroy_t destroy)
        : destroy(destroy) {}
    constexpr variant_t(change_t change)
        : change(change) {}
    constexpr variant_t(uint32_t data)
        : data(data) {}
    constexpr ClientFSM_Command GetCommand() const { return create.type.GetCommand(); }
    constexpr ClientFSM GetType() const { return create.type.GetType(); }
};
static_assert(int(ClientFSM_Command::none) == 0, "ClientFSM_Command::none must equal zero or fix variant_t::variant_t() ctor");

// Smart queue, discards events which can be discarded (no longer important events - if new event is inserted)
// for instance destroy, erases all other events in buffer, because they are no longer important (but nothing can erase destroy)
// merges multiple events into one ...
// but can never loose important item
// Originally wanted to store store openend state
// but it is dangerous - causing 1 information stored on multiple places
// wrong data input, can modify type (ClientFSM) of stored destroy command
class Queue {
protected:
    std::array<variant_t, 3> queue;
    uint8_t count;

    constexpr void clear() { count = 0; }
    constexpr void clear_last() {
        if (count)
            --count;
    }

    //this functions do not check validity of given argument !!! (public ones do)
    void push(variant_t v); // this method is called by specific ones (pushCreate ...)
    void pushCreate(create_t create);
    void pushDestroy(destroy_t destroy);
    void pushChange(change_t change);

public:
    constexpr Queue()
        : queue({ variant_t(0), variant_t(0), variant_t(0) })
        , count(0) {}

    variant_t Front() const; //returns ClientFSM_Command::none on empty
    variant_t Back() const;  //returns ClientFSM_Command::none on empty
    void Push(variant_t v);  // this method calls specific ones (PushCreate ...)
    void Pop();
    void PushCreate(ClientFSM type, uint8_t data);
    void PushDestroy(ClientFSM type);
    void PushChange(ClientFSM type, uint8_t phase, uint8_t progress_tot, uint8_t progress);
};

}; //namespace fsm
