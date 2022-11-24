/**
 * @file fsm_types.cpp
 * @author Radek Vana
 * @date 2021-02-18
 */

#include "fsm_types.hpp"
#include "log.h"

using namespace fsm;

LOG_COMPONENT_DEF(FSM, LOG_SEVERITY_INFO);

/**
 * @brief push create command into queue if able
 *
 * @param type fsm to be created
 * @param data fsm data
 * @return Queue::ret_val possible values
 *       - Queue::ret_val::ok
 *       - Queue::ret_val::er_already_created
 *       - Queue::ret_val::er_type_none
 *       - Queue::ret_val::er_opened_fsm_inconsistent
 */
Queue::ret_val Queue::PushCreate(ClientFSM type, uint8_t data) {
    if (type == ClientFSM::_none)
        return ret_val::er_type_none;
    return pushCreate(create_t(type, data));
}

/**
 * @brief push create command into queue if able, private version, does not check er_type_none
 * queue must be empty or last command must be destroy
 * no need to check size queue behavior does not allow push more than 3 items
 * @param create
 * @return Queue::ret_val possible values
 *       - Queue::ret_val::ok
 *       - Queue::ret_val::er_already_created
 *       - Queue::ret_val::er_opened_fsm_inconsistent
 */
Queue::ret_val Queue::pushCreate(create_t create) {
    if ((count == 0) || (Back().GetCommand() == ClientFSM_Command::destroy)) {
        if (opened_fsm != ClientFSM::_none) {
            return ret_val::er_opened_fsm_inconsistent;
        }
        push(variant_t(create)); // should always succeed
        opened_fsm = create.type.GetType();
        return ret_val::ok;
    }
    if (opened_fsm == ClientFSM::_none) {
        return ret_val::er_opened_fsm_inconsistent;
    }
    return ret_val::er_already_created;
}

/**
 * @brief push create destroy into queue if able
 *
 * @param type fsm to be destroyed
 * @return Queue::ret_val
 *       - Queue::ret_val::ok
 *       - Queue::ret_val::er_already_destroyed
 *       - Queue::ret_val::er_opened_fsm_inconsistent
 *       - Queue::ret_val::er_type_none
 */
Queue::ret_val Queue::PushDestroy(ClientFSM type) {
    if (type == ClientFSM::_none)
        return ret_val::er_type_none;
    if (type != opened_fsm) {
        return ret_val::er_opened_fsm_inconsistent;
    }
    return pushDestroy(destroy_t(type));
}

/**
 * @brief clear all commands and push destroy, does not check param validity
 * except when queue contains create - erase create instead clearing
 * @param destroy
 * @return Queue::ret_val possible values
 *       - Queue::ret_val::ok
 *       - Queue::ret_val::er_already_destroyed
 */
Queue::ret_val Queue::pushDestroy(destroy_t destroy) {
    if ((count == 1) && (queue[0].GetCommand() == ClientFSM_Command::destroy)) {
        return ret_val::er_already_destroyed; //do not modify stored destroy
    }

    if ((count > 0) && (queue[0].GetCommand() == ClientFSM_Command::create)) {
        Clear();
        opened_fsm = ClientFSM::_none;
        return ret_val::ok;
    }

    if ((count > 1) && (queue[1].GetCommand() == ClientFSM_Command::create)) {
        count = 1; // there is old destroy command in queue, leave it
        opened_fsm = ClientFSM::_none;
        return ret_val::ok;
    }

    Clear();
    push(variant_t(destroy)); // should always succeed, especially after Clear
    opened_fsm = ClientFSM::_none;
    return ret_val::ok;
}

Queue::ret_val Queue::PushChange(ClientFSM type, BaseData data) {
    if (type == ClientFSM::_none)
        return ret_val::er_type_none;
    if (type != opened_fsm) {
        return ret_val::er_opened_fsm_inconsistent;
    }
    return pushChange(change_t(type, data));
}

// must be empty or last command must create or change
// if openned ClientFSM must match
// no need to check size queue behavior does not allow push more than 3 items
Queue::ret_val Queue::pushChange(change_t change) {
    if (count == 0) {
        push(variant_t(change));
        return ret_val::ok;
    }

    if (Back().GetType() == change.type.GetType()) {
        if (Back().GetCommand() == ClientFSM_Command::create) {
            push(variant_t(change));
            return ret_val::ok;
        }

        if (Back().GetCommand() == ClientFSM_Command::change) {
            --count; // last is change of same type, must rewrite it
            push(variant_t(change));
            return ret_val::ok;
        }
    }
    return ret_val::er_opened_fsm_inconsistent;
}

void Queue::push(variant_t v) {
    if (count < queue.size()) {
        queue[count] = v;
        ++count;
    }
}

variant_t Queue::Front() const {
    if (count == 0)
        return variant_t();
    return queue[0];
}

variant_t Queue::Back() const {
    if (count == 0)
        return variant_t();
    return queue[count - 1];
}

bool Queue::Pop() {
    if (count == 0)
        return false;

    count -= 1;
    for (size_t i = 1; i <= count; ++i) {
        queue[i - 1] = queue[i];
    }
    return true;
}

variant_t SmartQueue::Front() const {
    if (prior_commands_in_queue0) {
        return queue0.Front();
    }
    variant_t ret = queue1.Front();
    if (ret.GetCommand() == ClientFSM_Command::none) {
        ret = queue0.Front();
    }
    return ret;
}

variant_t SmartQueue::Back() const {
    variant_t ret = queue0.Back();
    // prior_commands_in_queue0 has lower prior in this method
    // opposite functionality to front
    if ((queue0.GetCount() <= prior_commands_in_queue0) || (ret.GetCommand() == ClientFSM_Command::none)) {
        ret = queue1.Back();
    }
    return ret;
}

SmartQueue::Selector SmartQueue::Push(variant_t v) {
    switch (v.GetCommand()) {
    case ClientFSM_Command::create:
        return PushCreate(v.GetType(), v.create.data);
    case ClientFSM_Command::destroy:
        return PushDestroy(v.GetType());
    case ClientFSM_Command::change:
        return PushChange(v.GetType(), v.change.data);
    default:
        break;
    }
    return Selector::error;
}

SmartQueue::Selector SmartQueue::Pop() {
    if (prior_commands_in_queue0) {
        --prior_commands_in_queue0;
        return queue0.Pop() ? Selector::q0 : Selector::error;
    }
    if (queue1.Pop())
        return Selector::q1;

    return queue0.Pop() ? Selector::q0 : Selector::error;
}

SmartQueue::Selector SmartQueue::PushCreate(ClientFSM type, uint8_t data) {
    //error upper queue contains opened dialog
    if (queue1.GetOpenFsm() != ClientFSM::_none) {
        log_error(FSM, "Attempt to create 3rd level");
        return Selector::error;
    }

    // first try to push into bottom queue
    if (queue0.PushCreate(type, data) == Queue::ret_val::ok)
        return Selector::q0;

    prior_commands_in_queue0 = size_t(queue0.GetCreateIndex() + 1);
    return queue1.PushCreate(type, data) == Queue::ret_val::ok ? Selector::q1 : Selector::error;
}

SmartQueue::Selector SmartQueue::PushDestroy(ClientFSM type) {
    if (queue1.PushDestroy(type) == Queue::ret_val::ok) {
        //destroy can clear queue1, so prior_commands_in_queue0 must be cleared too
        if (queue1.GetCount() == 0) {
            prior_commands_in_queue0 = 0;
        }
        return Selector::q1;
    }
    return queue0.PushDestroy(type) == Queue::ret_val::ok ? Selector::q0 : Selector::error;
}

SmartQueue::Selector SmartQueue::PushChange(ClientFSM type, BaseData data) {
    if (queue1.PushChange(type, data) == Queue::ret_val::ok)
        return Selector::q1;

    return queue0.PushChange(type, data) == Queue::ret_val::ok ? Selector::q0 : Selector::error;
}

bool IQueueWrapper::pushCreate(SmartQueue *pQueues, size_t sz, ClientFSM type, uint8_t data, const char *fnc, const char *file, int line) {
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
