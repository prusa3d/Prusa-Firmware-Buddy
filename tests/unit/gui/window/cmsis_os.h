#pragma once
#include <stdint.h>

void __disable_irq();
void __enable_irq();
uint32_t HAL_GetTick();
