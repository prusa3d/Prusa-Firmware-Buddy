/// @file leds.h
#pragma once
#include "../config/config.h"
#include <stdint.h>

namespace modules {

/// @brief The leds namespace provides all necessary facilities related to the logical model of the sets of LEDs on the MMU unit.
///
/// We have 5 pairs of LEDs. In each pair there is a green and a red LED.
///
/// A typical scenario in the past was visualization of error states.
/// The combination of colors with blinking frequency had a specific meaning.
///
/// The physical connection is not important on this level (i.e. how and what shall be sent into the shift registers).
///
/// LEDS are physically connected to a pair of shift registers along with some other signals.
/// The physical write operations are handled by hal::shr16.
namespace leds {

/// Enum of LED modes
/// blink0 and blink1 allow for interlaced blinking of LEDs (one is on and the other off)
enum Mode {
    off,
    on,
    blink0, ///< start blinking at even periods
    blink1 ///< start blinking at odd periods
};

/// Enum of LEDs color - green or red
enum Color {
    red = 0,
    green = 1
};

/// A single LED
class LED {
public:
    constexpr inline LED() = default;

    /// Sets the mode of the LED
    /// @param mode to set
    void SetMode(leds::Mode mode);

    /// @returns the currently active mode of the LED
    inline leds::Mode Mode() const { return (leds::Mode)state.mode; }

    /// @returns true if the LED shines
    /// @param oddPeriod LEDs class operates this parameter based on blinking period based on elapsed real time
    bool Step(bool oddPeriod);

    /// @returns true if the LED shines
    inline bool On() const { return state.on; }

private:
    struct State {
        uint8_t on : 1;
        uint8_t mode : 2;
        constexpr inline State()
            : on(0)
            , mode(leds::Mode::off) {}
    };

    State state;
};

/// The main LEDs API takes care of the whole set of LEDs
class LEDs {
public:
    constexpr inline LEDs() = default;

    /// step LED automaton
    void Step();

    /// @returns the number of LED pairs
    inline constexpr uint8_t LedPairsCount() const { return ledPairs; }

    /// Sets the mode of a LED in a pair
    /// @param slot index of filament slot (index of the LED pair)
    /// @param color green or red LED
    /// @param mode to set
    inline void SetMode(uint8_t slot, Color color, Mode mode) {
        SetMode(slot * 2 + color, mode);
    }

    /// Sets the mode of a LED in a pair
    /// @param index - raw index of the LED in the internal leds array
    /// @param mode to set
    inline void SetMode(uint8_t index, Mode mode) {
        leds[index].SetMode(mode);
    }

    /// @returns the currently active mode of a LED in a pair
    /// @param slot index of filament slot (index of the LED pair)
    /// @param color green or red LED
    inline leds::Mode Mode(uint8_t slot, Color color) {
        return Mode(slot * 2 + color);
    }

    /// @returns the currently active mode of a LED
    /// @param index - raw index of the LED in the internal leds array
    inline leds::Mode Mode(uint8_t index) {
        return leds[index].Mode();
    }

    /// @returns true if a LED is shining
    /// @param index - raw index of the LED in the internal leds array
    inline bool LedOn(uint8_t index) const {
        return leds[index].On();
    }

    /// @returns true if a LED is shining
    /// @param slot index of filament slot (index of the LED pair)
    /// @param color green or red LED
    inline bool LedOn(uint8_t slot, Color color) const {
        return leds[slot * 2 + color].On();
    }

    /// Sets active slot LEDs to some mode and turns off all the others
    void SetPairButOffOthers(uint8_t activeSlot, modules::leds::Mode greenMode, modules::leds::Mode redMode);

    /// Turn off all LEDs
    void SetAllOff();

private:
    constexpr static const uint8_t ledPairs = config::toolCount;
    /// pairs of LEDs:
    /// [0] - green LED slot 0
    /// [1] - red LED slot 0
    /// [2] - green LED slot 1
    /// [3] - red LED slot 1
    /// [4] - green LED slot 2
    /// [5] - red LED slot 2
    /// [6] - green LED slot 3
    /// [7] - red LED slot 3
    /// [8] - green LED slot 4
    /// [9] - red LED slot 4
    LED leds[ledPairs * 2];

    /// Cache for avoiding duplicit writes into the shift registers (may reduce LED flickering on some boards)
    uint16_t cachedState = 0;
};

/// The one and only instance of FINDA in the FW
extern LEDs leds;

} // namespace LEDs
} // namespace modules

namespace ml = modules::leds;
