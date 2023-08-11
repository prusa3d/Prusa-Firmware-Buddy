#pragma once

#include <cstdlib>
#include <stdint.h>
#include <array>

#include "loadcell_shared.hpp"

namespace dwarf::loadcell {
void loadcell_init();
void loadcell_loop();
bool get_loadcell_sample(LoadcellRecord &sample);
void loadcell_irq();
void loadcell_set_enable(bool enable);
} // namespace dwarf::loadcell
