#pragma once
#include "hwio_pindef.h"

static void ext_mux_select(unsigned logic_input) {
    using namespace buddy::hw;
    muxSelectA.write(static_cast<Pin::State>(1u & logic_input));
    muxSelectB.write(static_cast<Pin::State>(1u & (logic_input >> 1)));
}
