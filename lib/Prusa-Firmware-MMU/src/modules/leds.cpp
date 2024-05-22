/// @file leds.cpp
#include "leds.h"
#include "../hal/shr16.h"
#include "timebase.h"

namespace modules {
namespace leds {

LEDs leds;

void LED::SetMode(leds::Mode mode) {
    state.mode = mode;
    // set initial state of LEDs correctly - transition from one mode to another
    switch (state.mode) {
    case leds::Mode::blink1:
    case leds::Mode::off:
        state.on = 0;
        break;

    case leds::Mode::blink0:
    case leds::Mode::on:
        state.on = 1;
        break;
    default:
        break;
    }
}

bool LED::Step(bool oddPeriod) {
    switch (state.mode) {
    // on and off don't change while stepping
    case leds::Mode::blink0:
        state.on = oddPeriod;
        break;
    case leds::Mode::blink1:
        state.on = !oddPeriod;
        break;
    default: // do nothing
        break;
    }
    return state.on;
}

void LEDs::Step() {
    uint16_t millis = mt::timebase.Millis();
    bool oddPeriod = ((millis / (config::ledBlinkPeriodMs / 2)) & 0x01U) != 0;
    uint16_t result = 0;
    for (uint8_t i = 0; i < ledPairs * 2; i++) {
        result <<= 1;
        result |= leds[i].Step(oddPeriod);
    }
    if (result != cachedState) {
        hal::shr16::shr16.SetLED(result);
        cachedState = result;
    }
}

void LEDs::SetPairButOffOthers(uint8_t activeSlot, ml::Mode greenMode, ml::Mode redMode) {
    SetAllOff();
    SetMode(activeSlot, ml::green, greenMode);
    SetMode(activeSlot, ml::red, redMode);
}

void LEDs::SetAllOff() {
    for (uint8_t i = 0; i < ledPairs; ++i) {
        SetMode(i, ml::green, ml::off);
        SetMode(i, ml::red, ml::off);
    }
}

} // namespace leds
} // namespace modules
