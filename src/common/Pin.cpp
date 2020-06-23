/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#include "Pin.hpp"

Pin *Pin::last = nullptr;

Pin::Pin()
    : previous(last) {
    last = this;
}

void Pin::configure_all() {
    Pin *pin = last;
    for (; pin != nullptr; pin = pin->previous) {
        pin->configure();
    }
}
