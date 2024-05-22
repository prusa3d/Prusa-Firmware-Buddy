/// @file user_input.cpp
#include "user_input.h"
#include "buttons.h"

namespace modules {
namespace user_input {

UserInput userInput;

static_assert(config::buttonCount <= 8, "To use >8 buttons the lastButtonStates variable needs to occupy more bytes");
static_assert((uint8_t)mb::Left == (uint8_t)Event::Left, "Indices of buttons and events must match for optimization purposes");
static_assert((uint8_t)mb::Middle == (uint8_t)Event::Middle, "Indices of buttons and events must match for optimization purposes");
static_assert((uint8_t)mb::Right == (uint8_t)Event::Right, "Indices of buttons and events must match for optimization purposes");

void UserInput::StepOneButton(uint8_t button) {
    bool press = mb::buttons.ButtonPressed(button);
    if (press != LastButtonState(button)) {
        if (press) { // emit an event only if the previous button state was "not pressed"
            eventQueue.push(static_cast<Event>(button));
        }
        // flipping lastState is a bit speculative, but should be safe, because we are in the "press != lastState" branch
        FlipLastButtonState(button);
    }
}

void UserInput::Step() {
    // for(uint8_t button = 0; button < config::buttonCount; ++button)...
    // The usual style of iterating can be rewritten into a more cryptic but shorter one (saves 4B):
    for (uint8_t button = config::buttonCount; button; /* nothing */) {
        StepOneButton(--button);
    }
}

void UserInput::ProcessMessage(uint8_t ev) {
    eventQueue.push((Event)(ev | Event::FromPrinter));
}

Event UserInput::StripFromPrinterBit(uint8_t e) {
    e &= (uint8_t)(~Event::FromPrinter);
    return (Event)e;
}

Event UserInput::ConsumeEvent() {
    if (eventQueue.empty())
        return Event::NoEvent;
    if (printerInCharge) {
        Event rv = eventQueue.front();
        if (rv & Event::FromPrinter) {
            eventQueue.pop(rv);
            return StripFromPrinterBit(rv);
        }
        return Event::NoEvent;
    } else {
        Event rv;
        eventQueue.pop(rv);
        return StripFromPrinterBit(rv);
    }
}

Event UserInput::ConsumeEventForPrinter() {
    if (eventQueue.empty())
        return Event::NoEvent;
    Event rv = eventQueue.front();
    if (rv & Event::FromPrinter) {
        // do not send the same buttons back but leave them in the queue for the MMU FW
        return Event::NoEvent;
    }
    eventQueue.pop(rv);
    return StripFromPrinterBit(rv);
}

void UserInput::Clear() {
    eventQueue.reset();
}

} // namespace user_input
} // namespace modules
