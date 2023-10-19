/**
 * @file fsm_types.cpp
 * @author Radek Vana
 * @date 2021-02-18
 */

#include "fsm_types.hpp"
#include "log.h"
#include "bsod.h"

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

namespace {
const char *to_string(ClientFSM type) {
    switch (type) {
    case ClientFSM::Serial_printing:
        return "Serial_printing";
    case ClientFSM::Load_unload:
        return "Load_unload";
    case ClientFSM::Preheat:
        return "Preheat";
    case ClientFSM::Selftest:
        return "Selftest";
    case ClientFSM::ESP:
        return "ESP";
    case ClientFSM::Printing:
        return "Printing";
    case ClientFSM::CrashRecovery:
        return "CrashRecovery";
    case ClientFSM::QuickPause:
        return "QuickPause";
    case ClientFSM::PrintPreview:
        return "PrintPreview";
    case ClientFSM::_none:
        return "none";
    }
    return "ERROR MEMORY CORRUPTED"; // cannot normally happen
}
} // namespace

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
    if (type == ClientFSM::_none) {
        return QueueRetVal::er_type_none;
    }

    // fsm already created
    if (has_opened_fsm()) {
        return QueueRetVal::er_already_created;
    }

    Change change = Change(index, type, data);

    // in the case there was the same FSM with the same data
    // and it was destroyed, but destroy was not send
    // just erase that destroy
    if (last_sent_data == change) {
        data_to_send = std::nullopt;
        return QueueRetVal::ok;
    }

    // last_sent_data does not change now, it changes after command send
    data_to_send = change;
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
    if (type == ClientFSM::_none) {
        return QueueRetVal::er_type_none;
    }

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
    if (type == ClientFSM::_none) {
        return QueueRetVal::er_type_none;
    }

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
    if (change.get_queue_index() != index) {
        return;
    }

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

bool Queue::has_opened_fsm() const {
    return get_opened_fsm() != ClientFSM::_none;
}

bool Queue::has_pending_create_command() const {
    return data_to_send // we have data to send
        && (data_to_send->get_fsm_type() != ClientFSM::_none) // ClientFSM::_none would be destroy command
        && (data_to_send->get_fsm_type() != last_sent_data.get_fsm_type()); // the same fsm type would be change command
}

size_t Queue::count() const {
    return data_to_send ? 1 : 0;
}

/*****************************************************************************/
// SmartQueue

/**
 * @brief deques one command
 * Q1 has higher priority
 * with only one exception - when Q0 has pending create
 *                         - create command insertion logic prevents Q1 inserting create to Q0 while Q1 has open FSM
 *
 * @return std::optional<DequeStates> command to be send to another thread
 */
std::optional<DequeStates> SmartQueue::dequeue() {
    // create from Q0 is prioritized
    if (queue0.has_pending_create_command()) {
        return queue0.dequeue();
    }

    // can have data but no open fsm (contains destroy command)
    if (queue1.count() || queue1.has_opened_fsm()) {
        return queue1.dequeue();
    }
    return queue0.dequeue();
}

SmartQueue::Selector SmartQueue::PushCreate(ClientFSM type, BaseData data) {
    // error upper queue contains opened dialog
    if (queue1.has_opened_fsm()) {
        bsod("FSM: Attempt to create 3rd level");
    }

    // error create cannot have type none
    if (type == ClientFSM::_none) {
        bsod("FSM: Cannot create ClientFSM::_none");
    }

    // try to push into bottom queue before top one
    if (queue0.push_create(type, data) == QueueRetVal::ok) {
        return Selector::q0;
    }

    if (queue1.push_create(type, data) == QueueRetVal::ok) {
        return Selector::q1;
    }

    bsod("FSM Cannot push create of %s to Q1", to_string(type));
}

SmartQueue::Selector SmartQueue::PushDestroy(ClientFSM type) {
    if (queue1.has_opened_fsm()) {
        if (queue1.push_destroy(type) == QueueRetVal::ok) {
            return Selector::q1;
        } else {
            bsod("FSM Cannot push destroy of %s to Q1", to_string(type));
        }
    }
    if (queue0.push_destroy(type) == QueueRetVal::ok) {
        return Selector::q0;
    }

    bsod("FSM Cannot push destroy of %s to Q0", to_string(type));
}

SmartQueue::Selector SmartQueue::PushChange(ClientFSM type, BaseData data) {
    if (queue1.push_change(type, data) == QueueRetVal::ok) {
        return Selector::q1;
    }

    if (queue0.push_change(type, data) != QueueRetVal::ok) {
        bsod("FSM CHANGE: cannot push %s", to_string(type));
    }

    return Selector::q0;
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

void IQueueWrapper::pushCreate(SmartQueue *pQueues, size_t sz, ClientFSM type, BaseData data, const char *fnc, const char *file, int line) {
    if (!pQueues || sz == 0) {
        bsod(""); // should never happen, no text to save CPU flash
    }

    if (fsm0 == type) {
        bsod("FSM CREATE: %s already opened at level 0. Called from %s, %s, ln %i", to_string(type), fnc, file, line);
    }

    if (fsm1 == type) {
        bsod("FSM CREATE: %s already opened at level 1. Called from %s, %s, ln %i", to_string(type), fnc, file, line);
    }

    log_info(FSM, "CREATE [%s]: %s, %s, ln %i", to_string(type), fnc, file, line);
    fsm_last_phase[static_cast<int>(type)] = -1;

    for (size_t i = 0; i < sz; ++i) {
        switch (pQueues[i].PushCreate(type, data)) {
        case SmartQueue::Selector::q0:
            fsm0 = type;
            break;
        case SmartQueue::Selector::q1:
            fsm1 = type;
            break;
        }
    }
}

void IQueueWrapper::pushDestroy(SmartQueue *pQueues, size_t sz, ClientFSM type, const char *fnc, const char *file, int line) {
    if (!pQueues || sz == 0) {
        bsod(""); // should never happen, no text to save CPU flash
    }

    if (fsm0 != type && fsm1 != type) {
        bsod("FSM DESTROY: %s does not exist. Called from %s, %s, ln %i", to_string(type), fnc, file, line);
    }

    if (fsm1 != type && fsm1 != ClientFSM::_none) {
        bsod("FSM DESTROY: %s blocked by higher level. Called from %s, %s, ln %i", to_string(type), fnc, file, line);
    }

    log_info(FSM, "DESTROY [%s]: %s, %s, ln %i", to_string(type), fnc, file, line);

    for (size_t i = 0; i < sz; ++i) {
        switch (pQueues[i].PushDestroy(type)) {
        case SmartQueue::Selector::q0:
            fsm0 = ClientFSM::_none;
            break;
        case SmartQueue::Selector::q1:
            fsm1 = ClientFSM::_none;
            break;
        }
    }
}

void IQueueWrapper::pushChange(SmartQueue *pQueues, size_t sz, ClientFSM type, BaseData data, const char *fnc, const char *file, int line) {
    if (!pQueues || sz == 0) {
        bsod(""); // should never happen, no text to save CPU flash
    }

    // check if given FSM type is stored in any queue
    if (fsm0 != type && fsm1 != type) {
        bsod("FSM CHANGE %s mismatch. Called from %s, %s, ln %i", to_string(type), fnc, file, line);
    }

    if (fsm_last_phase[static_cast<int>(type)] != static_cast<int>(data.GetPhase())) {
        log_info(FSM, "CHANGE [%s] to %" PRIu8 " %s, %s, ln %i", to_string(type), data.GetPhase(), fnc, file, line);
        fsm_last_phase[static_cast<int>(type)] = static_cast<int>(data.GetPhase());
    }

    // store data into corresponding queue
    // only highest level queue is being sent
    for (size_t i = 0; i < sz; ++i) {
        pQueues[i].PushChange(type, data);
    }
}
