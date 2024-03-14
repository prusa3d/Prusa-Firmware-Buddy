/**
 * @file fsensor_pins.hpp
 * @author Radek Vana
 * @brief api header, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

// cannot use anonymous namespace on this stupid header, idk why
#include "hwio_pindef.h"
#include <device/board.h>

class FSensorPins {
public:
    inline static void pullUp() { buddy::hw::fSensor.pullUp(); }
    inline static void pullDown() { buddy::hw::fSensor.pullDown(); }
    inline static bool Get() {

#if BOARD_IS_XBUDDY
        return buddy::hw::fSensor.read() == buddy::hw::Pin::State::low;
#else
        return buddy::hw::fSensor.read() == buddy::hw::Pin::State::high;
#endif
    }
};
