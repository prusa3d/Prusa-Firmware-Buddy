/**
 * @file fsm_types.cpp
 * @author Radek Vana
 * @date 2021-02-18
 */

#include "fsm_types.hpp"
#include "log.h"

using namespace fsm;

LOG_COMPONENT_DEF(FSM, LOG_SEVERITY_INFO);

std::pair<uint32_t, uint16_t> Change::serialize() const {
    uint16_t u16_lo = 0;
    uint16_t u16_hi = 0;
    uint32_t u32 = 0;
    uint8_t type_and_queue = static_cast<uint8_t>(type);
    type_and_queue |= uint8_t(queue_index) << 7;
    u16_hi = type_and_queue;
    u16_hi = u16_hi << 8;
    memcpy(&u32, &data, sizeof(u32));
    memcpy(&u16_lo, ((uint8_t *)(&data)) + sizeof(u32), 1);
    return { u32, u16_hi | u16_lo };
}

// deserialize ctor
Change::Change(std::pair<uint32_t, uint16_t> serialized)
    : type(static_cast<ClientFSM>((serialized.second >> 8) & 0x7F))
    , queue_index(static_cast<QueueIndex>((serialized.second >> 15) & 0x01)) {
    fsm::PhaseData phase_data;
    memcpy(&phase_data, &serialized.first, sizeof(serialized.first));
    data = { uint8_t(serialized.second), phase_data };
}

/*****************************************************************************/
// Queue

/**
 * @brief Removes and returns the object at the beginning of the Queue
 *
 * @return std::optional<Change>
 */
std::optional<DequeStates> Queue::dequeue() {
    if (!data_to_send) {
        return std::nullopt;
    }

    // no need to send anything
    if (last_sent_data == *data_to_send) {
        data_to_send = std::nullopt;
        return std::nullopt;
    }

    DequeStates ret(*data_to_send, last_sent_data);

    last_sent_data = *data_to_send;
    data_to_send = std::nullopt;
    return ret;
}

/**
 * @brief push create command into queue if able
 *
 * @param type fsm to be created
 * @param data fsm data
 * @return QueueRetVal possible values
 *       - QueueRetVal::ok
 *       - QueueRetVal::er_already_created
 *       - QueueRetVal::er_type_none
 */
QueueRetVal Queue::push_create(ClientFSM type, BaseData data) {
    if (type == ClientFSM::_none)
        return QueueRetVal::er_type_none;

    // fsm already created
    if (get_opened_fsm() != ClientFSM::_none) {
        return QueueRetVal::er_already_created;
    }

    // last_sent_data does not change now, it changes after command send
    data_to_send = Change(index, type, data);
    return QueueRetVal::ok;
}

/**
 * @brief push destroy command into queue if able
 *
 * @param type fsm to be destroyed
 * @return QueueRetVal
 *       - QueueRetVal::ok
 *       - QueueRetVal::er_opened_fsm_inconsistent
 *       - QueueRetVal::er_type_none
 */
QueueRetVal Queue::push_destroy(ClientFSM type) {
    if (type == ClientFSM::_none)
        return QueueRetVal::er_type_none;

    // wrong type
    if (get_opened_fsm() != type) {
        return QueueRetVal::er_opened_fsm_inconsistent;
    }

    if (last_sent_data.get_fsm_type() == ClientFSM::_none) {
        // create was not send yet
        // just clear the queue
        data_to_send = std::nullopt;
    } else {
        // some create was already send, store destroy into queue
        // ClientFSM does not matter, previous FSM would have been destroyed to be in new one
        // in that case new destroy would erase new FSM and leave old destroy
        // so we just store destroy in both cases (last_sent_data.get_fsm_type() != type OR last_sent_data.get_fsm_type() == type)

        // last_sent_data does not change now, it changes after command send
        data_to_send = Change(index); // ClientFSM::_none
    }

    return QueueRetVal::ok;
}

/**
 * @brief push change command into queue if able
 *
 * @param type fsm to be changed
 * @return QueueRetVal
 *       - QueueRetVal::ok
 *       - QueueRetVal::er_opened_fsm_inconsistent
 *       - QueueRetVal::er_type_none
 */
QueueRetVal Queue::push_change(ClientFSM type, BaseData data) {
    if (type == ClientFSM::_none)
        return QueueRetVal::er_type_none;

    // wrong type
    if (get_opened_fsm() != type) {
        return QueueRetVal::er_opened_fsm_inconsistent;
    }

    // last_sent_data does not change now, it changes after command send
    data_to_send = Change(index, type, data);
    return QueueRetVal::ok;
}

/**
 * @brief atomic operation to change 1 fsm to another (avoids flickering)
 *
 * @param old_type fsm to be destroyed
 * @param new_type fsm to be created
 * @param data     new fsm data
 * @return QueueRetVal, any return value from push_crete and push_destroy
 */
QueueRetVal Queue::push_destroy_and_create(ClientFSM old_type, ClientFSM new_type, BaseData data) {
    QueueRetVal ret = push_destroy(old_type);
    if (ret != QueueRetVal::ok) {
        return ret;
    }
    return push_create(new_type, data);
}

/**
 * @brief push data into queue
 * ignore any checks except index
 * @param change data to push
 */
void Queue::force_push(Change change) {
    if (change.get_queue_index() != index)
        return;

    data_to_send = change;
}

ClientFSM Queue::get_opened_fsm() const {
    if (data_to_send) {
        // data_to_send -> fsm stored in last_sent_data might not be valid
        return data_to_send->get_fsm_type();
    }

    // no data_to_send -> fsm stored in last_sent_data is valid
    return last_sent_data.get_fsm_type();
}

bool Queue::has_pending_create_command() const {
    return data_to_send && (data_to_send->get_fsm_type() != ClientFSM::_none) && (data_to_send->get_fsm_type() != last_sent_data.get_fsm_type());
}

size_t Queue::count() const {
    return data_to_send ? 1 : 0;
}

/*****************************************************************************/
// SmartQueue

std::optional<DequeStates> SmartQueue::dequeue() {
    if (prior_command_in_queue0) {
        prior_command_in_queue0 = false;
        return queue0.dequeue();
    }

    // can have data but no open fsm (contains destroy command)
    if (queue1.count() || queue1.get_opened_fsm() != ClientFSM::_none) {
        return queue1.dequeue();
    }
    return queue0.dequeue();
}

SmartQueue::Selector SmartQueue::PushCreate(ClientFSM type, BaseData data) {
    //error upper queue contains opened dialog
    if (queue1.get_opened_fsm() != ClientFSM::_none) {
        log_error(FSM, "Attempt to create 3rd level");
        return Selector::error;
    }

    // first try to push into bottom queue
    if (queue0.push_create(type, data) == QueueRetVal::ok)
        return Selector::q0;

    prior_command_in_queue0 = queue0.has_pending_create_command();
    return queue1.push_create(type, data) == QueueRetVal::ok ? Selector::q1 : Selector::error;
}

SmartQueue::Selector SmartQueue::PushDestroy(ClientFSM type) {
    if (queue1.push_destroy(type) == QueueRetVal::ok) {
        //destroy can clear queue1, so prior_commands_in_queue0 must be cleared too
        if (queue1.count() == 0) {
            prior_command_in_queue0 = false;
        }
        return Selector::q1;
    }
    return queue0.push_destroy(type) == QueueRetVal::ok ? Selector::q0 : Selector::error;
}

SmartQueue::Selector SmartQueue::PushChange(ClientFSM type, BaseData data) {
    if (queue1.push_change(type, data) == QueueRetVal::ok)
        return Selector::q1;

    return queue0.push_change(type, data) == QueueRetVal::ok ? Selector::q0 : Selector::error;
}

bool SmartQueue::TryPushCreate(ClientFSM type, BaseData data) {
    if (queue1.get_opened_fsm() == type || queue0.get_opened_fsm() == type) {
        return false;
    }

    Selector ret = PushCreate(type, data);

    return ret == Selector::q0 || ret == Selector::q1;
}

bool SmartQueue::TryPushDestroy(ClientFSM type) {
    if (queue1.get_opened_fsm() != type && queue0.get_opened_fsm() != type) {
        return false;
    }

    Selector ret = PushDestroy(type);

    return ret == Selector::q0 || ret == Selector::q1;
}

bool SmartQueue::TryPushChange(ClientFSM type, BaseData data) {
    Selector ret = PushChange(type, data);

    return ret == Selector::q0 || ret == Selector::q1;
}

/**
 * @brief push into queue by index
 * ignore any checks
 * @param change data to push
 */
void SmartQueue::force_push(Change change) {
    switch (change.get_queue_index()) {
    case QueueIndex::q0:
        queue0.force_push(change);
        return;
    case QueueIndex::q1:
        queue1.force_push(change);
        return;
    }
}

bool IQueueWrapper::pushCreate(SmartQueue *pQueues, size_t sz, ClientFSM type, BaseData data, const char *fnc, const char *file, int line) {
    if (!pQueues || sz == 0)
        return false;

    if (fsm0 == type) {
        log_error(FSM, "CREATE: %s, %s, ln %i, already opened at level 0", fnc, file, line);
        return false;
    }

    if (fsm1 == type) {
        log_error(FSM, "CREATE: %s, %s, ln %i, already opened at level 1", fnc, file, line);
        return false;
    }

    bool ret = true;

    log_info(FSM, "CREATE [%d]: %s, %s, ln %i", int(type), fnc, file, line);
    fsm_last_phase[static_cast<int>(type)] = -1;

    for (size_t i = 0; i < sz; ++i) {
        switch (pQueues[i].PushCreate(type, data)) {
        case SmartQueue::Selector::error:
            ret = false;
            log_error(FSM, "CREATE - failed on queue [%i]: %s, %s, ln %i", i, fnc, file, line);
            break;
        case SmartQueue::Selector::q0:
            fsm0 = type;
            break;
        case SmartQueue::Selector::q1:
            fsm1 = type;
            break;
        }
    }
    return ret;
}

bool IQueueWrapper::pushDestroy(SmartQueue *pQueues, size_t sz, ClientFSM type, const char *fnc, const char *file, int line) {
    if (!pQueues || sz == 0)
        return false;

    if (fsm0 != type && fsm1 != type) {
        log_error(FSM, "DESTROY - does not exist: %s, %s, ln %i", fnc, file, line);
        return false;
    }

    if (fsm1 != type && fsm1 != ClientFSM::_none) {
        log_error(FSM, "DESTROY - blocked by higher level FSM: %s, %s, ln %i", fnc, file, line);
        return false;
    }

    bool ret = true;

    log_info(FSM, "DESTROY [%d]: %s, %s, ln %i", int(type), fnc, file, line);

    for (size_t i = 0; i < sz; ++i) {
        switch (pQueues[i].PushDestroy(type)) {
        case SmartQueue::Selector::error:
            ret = false;
            log_error(FSM, "DESTROY - failed on queue [%i]: %s, %s, ln %i", i, fnc, file, line);
            break;
        case SmartQueue::Selector::q0:
            fsm0 = ClientFSM::_none;
            break;
        case SmartQueue::Selector::q1:
            fsm1 = ClientFSM::_none;
            break;
        }
    }
    return ret;
}

bool IQueueWrapper::pushChange(SmartQueue *pQueues, size_t sz, ClientFSM type, BaseData data, const char *fnc, const char *file, int line) {
    if (!pQueues || sz == 0)
        return false;
    bool ret = true;

    // top level type is wrong (none is ok) or required type is in neither queue
    if ((fsm1 != type && fsm1 != ClientFSM::_none) || (fsm0 != type && fsm1 != type)) {
        log_error(FSM, "CHANGE type mismatch: %s, %s, ln %i", fnc, file, line);
        return false;
    }

    if (fsm_last_phase[static_cast<int>(type)] != static_cast<int>(data.GetPhase())) {
        log_info(FSM, "CHANGE of [%i] to %" PRIu8 " %s, %s, ln %i", static_cast<int>(type), data.GetPhase(), fnc, file, line);
        fsm_last_phase[static_cast<int>(type)] = static_cast<int>(data.GetPhase());
    }

    for (size_t i = 0; i < sz; ++i) {
        if (pQueues[i].PushChange(type, data) == SmartQueue::Selector::error) {
            ret = false;
            log_error(FSM, "CHANGE failed on queue [%i]: %s, %s, ln %i", i, fnc, file, line);
        }
    }
    return ret;
}
