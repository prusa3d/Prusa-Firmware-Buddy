/// @file user_input.h
#pragma once
#include <stdint.h>
#include "../hal/circular_buffer.h"

namespace modules {

/// User input module collects input from buttons and from communication for the logic layer
namespace user_input {

/// Beware - button codes intentionally match the protocol button encoding for optimization purposes
enum Event : uint8_t {
    NoEvent = 0x7f,
    Right = 0,
    Middle = 1,
    Left = 2,
    FromPrinter = 0x80
};

class UserInput {

public:
    UserInput() = default;

    /// collects the buttons' state and enqueues the corresponding event
    void Step();

    /// enqueues a user event coming from a communication
    void ProcessMessage(uint8_t ev);

    /// Dequeues the most recent event from the queue for processing.
    /// This is what all the state machines shall call as is encapsulates
    /// the logic of not providing the buttons for the MMU FW in case the printer
    /// is in charge of UI processing.
    Event ConsumeEvent();

    /// Dequeues the most recent event from the queue for sending to the printer.
    /// Beware - use this call wisely, it is intended only for extraction of events to be sent to the printer.
    Event ConsumeEventForPrinter();

    /// @returns true if there is at least one event in the event queue
    bool AnyEvent() const { return !eventQueue.empty(); }

    /// Remove all buffered events from the event queue
    void Clear();

    /// Sets the UI module into a mode when the printer is in charge of processing the buttons (from all sources).
    /// That means the MMU will detect its buttons but it will not react upon them.
    /// This mode is important for error recovery when the printer needs to do some stuff before the MMU (like preheating the nozzle).
    inline void SetPrinterInCharge(bool pc) {
        printerInCharge = pc;
    }

    /// @returns true if the printer is in charge of the buttons
    inline bool PrinterInCharge() const {
        return printerInCharge;
    }

#ifndef UNITTEST
private:
#endif
    CircularBuffer<Event, uint_fast8_t, 4> eventQueue;
    bool printerInCharge = false;
    uint8_t lastButtonStates = 0; ///< one bit per button, 1 means pressed

    constexpr bool LastButtonState(uint8_t button) const {
        return (lastButtonStates & (1 << button)) != 0;
    }
    void FlipLastButtonState(uint8_t button) {
        lastButtonStates ^= (1 << button);
    }
    void StepOneButton(uint8_t button);

    static Event StripFromPrinterBit(uint8_t e);
};

extern UserInput userInput;

} // namespace user_input
} // namespace modules

namespace mui = modules::user_input;
