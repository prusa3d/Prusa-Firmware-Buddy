/// @file buttons.cpp
#include "buttons.h"
#include "../hal/adc.h"
#include "timebase.h"

namespace modules {
namespace buttons {

Buttons buttons;

int8_t Buttons::DecodeADC(uint16_t rawADC) {
    // decode 3 buttons' levels from one ADC
    // Button 1 - 0
    // Button 2 - 90
    // Button 3 - 170
    // Doesn't handle multiple pressed buttons at once

    for (int8_t buttonIndex = 0; buttonIndex < config::buttonCount; ++buttonIndex) {
        if (rawADC >= config::buttonADCLimits[buttonIndex][0] && rawADC <= config::buttonADCLimits[buttonIndex][1])
            return buttonIndex;
    }

    return -1;
}

void Buttons::Step() {
    uint16_t millis = mt::timebase.Millis();
    int8_t currentState = DecodeADC(hal::adc::ReadADC(config::buttonsADCIndex));
    for (uint_fast8_t b = 0; b < config::buttonCount; ++b) {
        // this button was pressed if b == currentState, released otherwise
        buttons[b].Step(millis, b == currentState);
    }
}

} // namespace buttons
} // namespace modules
