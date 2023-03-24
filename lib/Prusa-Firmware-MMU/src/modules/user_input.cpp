/// @file user_input.cpp
#include "user_input.h"
#include "buttons.h"

namespace modules {
namespace user_input {

UserInput userInput;

void UserInput::Step() {
    if (buttons::buttons.ButtonPressed(mb::Left))
        eventQueue.push(Event::Left);
    if (buttons::buttons.ButtonPressed(mb::Middle))
        eventQueue.push(Event::Middle);
    if (buttons::buttons.ButtonPressed(mb::Right))
        eventQueue.push(Event::Right);
}

void UserInput::ProcessMessage(uint8_t ev) {
    eventQueue.push((Event)ev);
}

Event UserInput::ConsumeEvent() {
    if (eventQueue.empty())
        return Event::NoEvent;
    Event rv;
    eventQueue.pop(rv);
    return rv;
}

void UserInput::Clear() {
    while (!eventQueue.empty()) {
        Event x;
        eventQueue.pop(x);
    }
}

} // namespace user_input
} // namespace modules
