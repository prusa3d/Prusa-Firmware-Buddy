#pragma once

#include <cstdlib>
#include <stdint.h>
#include <array>

#include "puppies/fifo_coder.hpp"

namespace dwarf::loadcell {
using namespace common::puppies::fifo;
void loadcell_init();
void loadcell_loop();
bool get_loadcell_sample(LoadcellRecord &sample);
void loadcell_irq();
void loadcell_set_enable(bool enable);
} // namespace dwarf::loadcell
