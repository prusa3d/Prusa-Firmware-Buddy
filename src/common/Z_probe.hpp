/**
 * @file
 */

#pragma once

#include <stdint.h>

uint32_t get_Z_probe_endstop_hits();

namespace buddy::hw {
void Z_probe_interrupt_handler();

}
