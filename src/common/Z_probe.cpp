/**
 * @file
 * @date Nov 12, 2021
 * @author Marek Bel
 */
#include "Z_probe.hpp"
#include "hwio_pindef.h"

static volatile uint32_t minda_falling_edges = 0;
uint32_t get_Z_probe_endstop_hits() {
    return minda_falling_edges;
}

void buddy::hw::Z_probe_interrupt_handler() {
    using namespace buddy::hw;

    const Pin::State currentState = zMin.read();
    static Pin::State previousState = currentState;

    if ((Pin::State::low == currentState) && (currentState != previousState)) {
        ++minda_falling_edges;
    }

    previousState = currentState;
}
