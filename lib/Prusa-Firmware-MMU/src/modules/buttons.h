/// @file buttons.h
#pragma once

#include <stdint.h>
#include "debouncer.h"
#include "../config/config.h"

/// The modules namespace contains models of MMU's components
namespace modules {

/// The buttons namespace provides all necessary facilities related to the logical model of the physical buttons device the MMU unit.
namespace buttons {

/// A model of a single button, performs automatic debouncing on top of the raw ADC API
struct Button : public debounce::Debouncer {
    inline constexpr Button()
        : debounce::Debouncer(config::buttonsDebounceMs) {}

private:
};

/// Enum of buttons - used also as indices in an array of buttons to keep the code size tight.
enum {
    Right = 0,
    Middle,
    Left
};

/// A model of the 3 buttons on the MMU unit
class Buttons {
public:
    inline constexpr Buttons() = default;

    /// Performs one step of the state machine - reads the ADC, processes debouncing, updates states of individual buttons
    void Step();

    /// @returns true if button at index is pressed
    /// @param index of the button to check
    inline bool ButtonPressed(uint8_t index) const { return buttons[index].Pressed(); }

    /// @returns true if any of the button is pressed
    inline bool AnyButtonPressed() const {
        for (uint8_t i = 0; i < config::buttonCount; ++i) {
            if (ButtonPressed(i))
                return true;
        }
        return false;
    }

private:
    Button buttons[config::buttonCount];

    /// Decode ADC output into a button index
    /// @returns index of the button pressed or -1 in case no button is pressed
    static int8_t DecodeADC(uint16_t rawADC);
};

/// The one and only instance of Buttons in the FW
extern Buttons buttons;

} // namespace buttons
} // namespace modules

namespace mb = modules::buttons;
