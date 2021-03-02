/**
 * @file fsm_types.cpp
 * @author Radek Vana
 * @date 2021-02-18
 */

#include "fsm_types.hpp"
using namespace fsm;

void Queue::PushCreate(ClientFSM type, uint8_t data) {
    if (type == ClientFSM::_none)
        return;
    pushCreate(create_t(type, data));
}

// queue must be empty or last command must be destroy
// no need to check size queue behavior does not allow push more than 3 items
void Queue::pushCreate(create_t create) {
    if ((count == 0) || (Back().GetCommand() == ClientFSM_Command::destroy)) {
        push(variant_t(create));
    }
}

void Queue::PushDestroy(ClientFSM type) {
    if (type == ClientFSM::_none)
        return;
    pushDestroy(destroy_t(type));
}

// clear all commands and push destroy
// except when queue contains create, type does not matter - because sequence create-destroy can never be stored in queue
void Queue::pushDestroy(destroy_t destroy) {
    if ((count == 1) && (queue[0].GetCommand() == ClientFSM_Command::destroy)) {
        return; //do not modify stored destroy
    }

    if ((count > 0) && (queue[0].GetCommand() == ClientFSM_Command::create)) {
        clear();
        return;
    }

    if ((count > 1) && (queue[1].GetCommand() == ClientFSM_Command::create)) {
        count = 1; // there is old destroy command in queue, leave it
        return;
    }

    clear();
    push(variant_t(destroy));
}

void Queue::PushChange(ClientFSM type, BaseData data) {
    if (type == ClientFSM::_none)
        return;
    pushChange(change_t(type, data));
}

// must be empty or last command must create or change
// if openned ClientFSM must match
// no need to check size queue behavior does not allow push more than 3 items
void Queue::pushChange(change_t change) {
    if (count == 0) {
        push(variant_t(change));
        return;
    }

    if (Back().GetType() == change.type.GetType()) {
        if (Back().GetCommand() == ClientFSM_Command::create) {
            push(variant_t(change));
            return;
        }

        if (Back().GetCommand() == ClientFSM_Command::change) {
            --count; // last is change of same type, must rewrite it
            push(variant_t(change));
            return;
        }
    }
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

void Queue::Pop() {
    if (count == 0)
        return;

    count -= 1;
    for (size_t i = 1; i <= count; ++i) {
        queue[i - 1] = queue[i];
    }
}

void Queue::Push(variant_t v) {
    if (v.GetType() == ClientFSM::_none)
        return;

    switch (v.GetCommand()) {
    case ClientFSM_Command::create:
        pushCreate(v.create);
        break;
    case ClientFSM_Command::destroy:
        pushDestroy(v.destroy);
        break;
    case ClientFSM_Command::change:
        pushChange(v.change);
        break;
    default:
        break;
    }
}
