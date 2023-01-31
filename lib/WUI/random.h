#pragma once

#include <cstdint>

// simple wrapper around HAL_RNG_GenerateRandomNumber,
// exists just so we can injest mock random for testing purposes
bool random32bit(uint32_t *random);
