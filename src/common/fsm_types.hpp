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
#include "fsm_base_types.hpp"

namespace fsm {
#pragma pack(push, 1) // must be packed to fit in variant8

struct type_t {
    uint8_t command_and_type;
    constexpr type_t(ClientFSM_Command command, ClientFSM type)
        : command_and_type(uint8_t(type) | uint8_t(command)) {}
    constexpr ClientFSM_Command GetCommand() const { return ClientFSM_Command(command_and_type & uint8_t(ClientFSM_Command::_mask)); }
    constexpr ClientFSM GetType() const { return ClientFSM(command_and_type & (~uint8_t(ClientFSM_Command::_mask))); }
    constexpr bool operator==(const type_t &other) const {
        return command_and_type == other.command_and_type;
    }
    constexpr bool operator!=(const type_t &other) const {
        return !((*this) == other);
    }
};
static_assert(sizeof(type_t) == 1, "Wrong size of type_t");

struct create_t {
    type_t type;
    uint8_t data;
    constexpr create_t(ClientFSM type_, uint8_t data)
        : type(type_t(ClientFSM_Command::create, type_))
        , data(data) {}
    constexpr bool operator==(const create_t &other) const {
        return (type == other.type) && (data == other.data);
    }
    constexpr bool operator!=(const create_t &other) const {
        return !((*this) == other);
    }
};
static_assert(sizeof(create_t) <= BaseDataSZ + sizeof(type_t), "Wrong size of create_t");

struct destroy_t {
    type_t type;
    constexpr destroy_t(ClientFSM type_)
        : type(type_t(ClientFSM_Command::destroy, type_)) {}
    constexpr bool operator==(const destroy_t &other) const {
        return type == other.type;
    }
    constexpr bool operator!=(const destroy_t &other) const {
        return !((*this) == other);
    }
};
static_assert(sizeof(destroy_t) <= BaseDataSZ + sizeof(type_t), "Wrong size of destroy_t");

struct change_t {
    type_t type;
    BaseData data;
    constexpr change_t(ClientFSM type_, BaseData data)
        : type(type_t(ClientFSM_Command::change, type_))
        , data(data) {}
    constexpr bool operator==(const change_t &other) const {
        return (type == other.type) && (data == other.data);
    }
    constexpr bool operator!=(const change_t &other) const {
        return !((*this) == other);
    }
};
static_assert(sizeof(change_t) <= BaseDataSZ + sizeof(type_t), "Wrong size of change_t");

union variant_t {
    create_t create;
    destroy_t destroy;
    change_t change;
    struct {
        uint32_t u32;
        uint16_t u16;
    };
    constexpr variant_t()
        : u32(0) //contains ClientFSM_Command::none
        , u16(0) {}
    constexpr variant_t(uint32_t u32, uint16_t u16)
        : u32(u32)
        , u16(u16) {}
    constexpr variant_t(create_t create_)
        : variant_t() // avoid uninitialized warning when accessing u16
    { create = create_; }
    constexpr variant_t(destroy_t destroy_)
        : variant_t() // avoid uninitialized warning when accessing u16
    { destroy = destroy_; }
    constexpr variant_t(change_t change)
        : change(change) {}
    constexpr ClientFSM_Command GetCommand() const { return create.type.GetCommand(); }
    constexpr ClientFSM GetType() const { return create.type.GetType(); }

    constexpr bool operator==(const variant_t &other) const {
        if (GetCommand() != other.GetCommand())
            return false;
        switch (GetCommand()) {
        case ClientFSM_Command::create:
            return create == other.create;
        case ClientFSM_Command::destroy:
            return destroy == other.destroy;
        case ClientFSM_Command::change:
            return change == other.change;
        default:
            return false;
        }
    }
    constexpr bool operator!=(const variant_t &other) const {
        return !((*this) == other);
    }
};
static_assert(int(ClientFSM_Command::none) == 0, "ClientFSM_Command::none must equal zero or fix variant_t::variant_t() ctor");
static_assert(sizeof(variant_t) == BaseDataSZ + sizeof(type_t), "Wrong size of variant_t");

#pragma pack(pop)

/**
 * @brief Smart queue, discards events which can be discarded (no longer important events - if new event is inserted)
 * for instance destroy, erases all other events in buffer, because they are no longer important (but nothing can erase destroy)
 * merges multiple events into one, but can never loose important item
 */
class Queue {
public:
    enum class ret_val {
        ok,
        //errors
        er_type_none,
        er_already_created,
        er_already_destroyed,
        er_opened_fsm_inconsistent,
    };

private:
    std::array<variant_t, 3> queue;
    uint8_t count;
    ClientFSM opened_fsm;

    //this functions do not check validity of given argument !!! (public ones do)
    void push(variant_t v); // this method is called by specific ones (pushCreate ...)
    ret_val pushCreate(create_t create);
    ret_val pushDestroy(destroy_t destroy);
    ret_val pushChange(change_t change);

public:
    constexpr Queue()
        : queue({ variant_t(), variant_t(), variant_t() })
        , count(0)
        , opened_fsm(ClientFSM::_none) {}

    variant_t Front() const; // returns ClientFSM_Command::none on empty
    variant_t Back() const;  // returns ClientFSM_Command::none on empty
                             // void Push(variant_t v);  // this method calls specific ones (PushCreate ...)
    bool Pop();
    ret_val PushCreate(ClientFSM type, uint8_t data);
    ret_val PushDestroy(ClientFSM type);
    ret_val PushChange(ClientFSM type, BaseData data);

    constexpr void Clear() { count = 0; }
    constexpr void Clear_last() {
        if (count)
            --count;
    }
    constexpr ClientFSM GetOpenFsm() const { return opened_fsm; }
    constexpr size_t GetCount() const { return count; }
    int GetCreateIndex() const {
        if ((count == 3) && (queue[2].GetCommand() == ClientFSM_Command::create)) {
            return 2;
        }
        if ((count >= 2) && (queue[1].GetCommand() == ClientFSM_Command::create)) {
            return 1;
        }
        if ((count >= 1) && (queue[0].GetCommand() == ClientFSM_Command::create)) {
            return 0;
        }
        return -1;
    }
};

/**
 * @brief 2nd level smart queue
 * contains 2 smart queues to support 2 level fsm nesting
 */
class SmartQueue {
protected:
    Queue queue0;                        // base queue
    Queue queue1;                        // next level queue
    size_t prior_commands_in_queue0 = 0; // when inserting create to queue1, last inserted create in queue0 has priority

    constexpr void clear() {
        queue0.Clear();
        queue1.Clear();
    }

public:
    enum class Selector { q0,
        q1,
        error,
        count_ = error }; // return type

    variant_t Front() const;    // returns ClientFSM_Command::none on empty
    variant_t Back() const;     // returns ClientFSM_Command::none on empty
    Selector Push(variant_t v); // this method calls specific ones (PushCreate ...)
    Selector Pop();
    Selector PushCreate(ClientFSM type, uint8_t data);
    Selector PushDestroy(ClientFSM type);
    Selector PushChange(ClientFSM type, BaseData data);
};

/**
 * @brie parent of template QueueWrapper
 */
class IQueueWrapper {
    ClientFSM fsm0 = ClientFSM::_none; // active fsm level 0
    ClientFSM fsm1 = ClientFSM::_none; // active fsm level 1

    /// Array of the last phase per fsm
    /// Used for better logging experience in fsm_change
    int fsm_last_phase[int(ClientFSM::_count)];

protected:
    bool pushCreate(SmartQueue *pQueues, size_t sz, ClientFSM type, uint8_t data, const char *fnc, const char *file, int line);
    bool pushDestroy(SmartQueue *pQueues, size_t sz, ClientFSM type, const char *fnc, const char *file, int line);
    bool pushChange(SmartQueue *pQueues, size_t sz, ClientFSM type, BaseData data, const char *fnc, const char *file, int line);

public:
    ClientFSM GetFsm0() const { return fsm0; }
    ClientFSM GetFsm1() const { return fsm1; }
};

/**
 * @brief wraps smart queues fo clients
 * to be used on server side only
 */
template <size_t SZ>
class QueueWrapper : public IQueueWrapper {
    SmartQueue queues[SZ];

public:
    bool PushCreate(ClientFSM type, uint8_t data, const char *fnc, const char *file, int line) {
        return pushCreate(queues, SZ, type, data, fnc, file, line);
    }
    bool PushDestroy(ClientFSM type, const char *fnc, const char *file, int line) {
        return pushDestroy(queues, SZ, type, fnc, file, line);
    }
    bool PushChange(ClientFSM type, BaseData data, const char *fnc, const char *file, int line) {
        return pushChange(queues, SZ, type, data, fnc, file, line);
    }

    variant_t Front(size_t queue) const { return queues[queue].Front(); }
    bool Pop(size_t queue) { return queues[queue].Pop() != SmartQueue::Selector::error; }
};

}; //namespace fsm
