/**
 * @file fsm_types.hpp
 * @author Radek Vana
 * @brief smart queue for fsm, stores and keeps only actual requests
 * @date 2021-02-18
 */

#pragma once

#include "client_fsm_types.h"
#include "fsm_base_types.hpp"

#include <stdint.h>
#include <array>
#include <optional>
#include <utils/utility_extensions.hpp>
#include <cstring> // memcpy

namespace fsm {

enum class QueueIndex : uint8_t {
    q0,
    q1,
    last_ = q1
};

class Change {
    ClientFSM type;
    QueueIndex queue_index;
    BaseData data;

public:
    constexpr Change(QueueIndex queue, ClientFSM fsm_type, BaseData data)
        : type(fsm_type)
        , queue_index(queue)
        , data(data) {}
    constexpr Change(QueueIndex queue)
        : type(ClientFSM::_none)
        , queue_index(queue) {}
    Change(std::pair<uint32_t, uint16_t> data); // deserialize ctor

    constexpr ClientFSM get_fsm_type() const { return type; }
    constexpr QueueIndex get_queue_index() const { return queue_index; }
    constexpr BaseData get_data() const { return data; }

    constexpr bool operator==(const Change &other) const {
        return (queue_index == other.queue_index) && (type == other.type) && ((type == ClientFSM::_none) || (data == other.data));
    }
    constexpr bool operator!=(const Change &other) const {
        return !((*this) == other);
    }

    std::pair<uint32_t, uint16_t> serialize() const;
};

enum class QueueRetVal {
    ok,
    // errors
    er_type_none,
    er_already_created,
    er_already_destroyed,
    er_opened_fsm_inconsistent,
};

struct DequeStates {
    Change current;
    Change last_sent;
};

/**
 * @brief we no longer transfers commands (any command is change)
 * also we now have single element queue
 */
class Queue {
    std::optional<Change> data_to_send; // single element queue, nullopt == empty
    Change last_sent_data; // not optional, set to ClientFSM::_none, changes during send, can prevent unnecessary send

    const QueueIndex index;

public:
    constexpr Queue(QueueIndex index_)
        : last_sent_data(index_)
        , index(index_) {}

    std::optional<DequeStates> dequeue();
    QueueRetVal push_create(ClientFSM type, BaseData data);
    QueueRetVal push_destroy(ClientFSM type);
    QueueRetVal push_change(ClientFSM type, BaseData data);
    QueueRetVal push_destroy_and_create(ClientFSM old_type, ClientFSM new_type, BaseData data);

    void force_push(Change change);

    ClientFSM get_opened_fsm() const;
    bool has_opened_fsm() const;
    bool has_pending_create_command() const;
    size_t count() const;
};

/**
 * @brief 2nd level smart queue
 * contains 2 smart queues to support 2 level fsm nesting
 */
class SmartQueue {
protected:
    Queue queue0 = { QueueIndex::q0 }; // base queue
    Queue queue1 = { QueueIndex::q1 }; // next level queue

public:
    enum class Selector { q0,
        q1 }; // return type

    std::optional<DequeStates> dequeue();

    Selector PushCreate(ClientFSM type, BaseData data);
    Selector PushDestroy(ClientFSM type);
    Selector PushChange(ClientFSM type, BaseData data);

    ClientFSM GetOpenFsmQ0() const { return queue0.get_opened_fsm(); }
    ClientFSM GetOpenFsmQ1() const { return queue1.get_opened_fsm(); }

    void force_push(Change change);
};

/**
 * @brief parent of template QueueWrapper
 */
class IQueueWrapper {
    ClientFSM fsm0 = ClientFSM::_none; // active fsm level 0
    ClientFSM fsm1 = ClientFSM::_none; // active fsm level 1

    /// Array of the last phase per fsm
    /// Used for better logging experience in fsm_change
    int fsm_last_phase[int(ClientFSM::_count)];

protected:
    void pushCreate(SmartQueue *pQueues, size_t sz, ClientFSM type, BaseData data, const char *fnc, const char *file, int line);
    void pushDestroy(SmartQueue *pQueues, size_t sz, ClientFSM type, const char *fnc, const char *file, int line);
    void pushChange(SmartQueue *pQueues, size_t sz, ClientFSM type, BaseData data, const char *fnc, const char *file, int line);

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
    void PushCreate(ClientFSM type, BaseData data, const char *fnc, const char *file, int line) {
        pushCreate(queues, SZ, type, data, fnc, file, line);
    }
    void PushDestroy(ClientFSM type, const char *fnc, const char *file, int line) {
        pushDestroy(queues, SZ, type, fnc, file, line);
    }
    void PushChange(ClientFSM type, BaseData data, const char *fnc, const char *file, int line) {
        pushChange(queues, SZ, type, data, fnc, file, line);
    }
    void PushDestroyAndCreate(ClientFSM old_type, ClientFSM new_type, BaseData data, const char *fnc, const char *file, int line) {
        pushDestroy(queues, SZ, old_type, fnc, file, line);
        pushCreate(queues, SZ, new_type, data, fnc, file, line);
    }

    std::optional<DequeStates> dequeue(size_t queue) { return queues[queue].dequeue(); };
};

}; // namespace fsm
