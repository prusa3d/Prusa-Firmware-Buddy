#pragma once
#include "stm32f4xx_hal.h"
// bitband macro
#define BITBAND_PERIPH(address, bit) ((uint8_t *)(PERIPH_BB_BASE + (((uint32_t)(address)) - PERIPH_BASE) * 32 + (bit)*4));
