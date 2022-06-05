/**
 * @file fsm_types.cpp
 * @author Radek Vana
 * @date 2021-02-18
 */

#include "fsm_types.hpp"
using namespace fsm;

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

void SmartQueue::Push(variant_t v) {
    switch (v.GetCommand()) {
    case ClientFSM_Command::create:
        PushCreate(v.GetType(), v.create.data);
        break;
    case ClientFSM_Command::destroy:
        PushDestroy(v.GetType());
        break;
    case ClientFSM_Command::change:
        PushChange(v.GetType(), v.change.data);
        break;
    default:
        break;
    }
}

void SmartQueue::Pop() {
    if (prior_commands_in_queue0) {
        --prior_commands_in_queue0;
        queue0.Pop();
    } else if (!queue1.Pop()) {
        queue0.Pop();
    }
}

void SmartQueue::PushCreate(ClientFSM type, uint8_t data) {
    //error upper queue contains openned dialog
    if (queue1.GetOpenFsm() != ClientFSM::_none) {
        return;
    }
    if (queue0.PushCreate(type, data) != Queue::ret_val::ok) {
        prior_commands_in_queue0 = size_t(queue0.GetCreateIndex() + 1);
        queue1.PushCreate(type, data);
    }
}

void SmartQueue::PushDestroy(ClientFSM type) {
    if (queue1.PushDestroy(type) != Queue::ret_val::ok) {
        queue0.PushDestroy(type);
    } else {
        //destroy can clear queue1, so prior_commands_in_queue0 must be cleared too
        if (queue1.GetCount() == 0) {
            prior_commands_in_queue0 = 0;
        }
    }
}

void SmartQueue::PushChange(ClientFSM type, BaseData data) {
    if (queue1.PushChange(type, data) != Queue::ret_val::ok) {
        queue0.PushChange(type, data);
    }
}
