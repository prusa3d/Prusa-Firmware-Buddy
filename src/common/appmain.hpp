#pragma once

#include <cstdint>

void app_assert(uint8_t *file, uint32_t line);
void app_error();
void app_run();
void app_tim14_tick();
